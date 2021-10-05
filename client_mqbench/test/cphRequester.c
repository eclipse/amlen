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
*          110513      fentono       File Creation
*/

/* This module is the implementation of the "Requester" test suite. This provides a set of functions that are used to put messages under
** the control of the worker thread function, which controls the rate and duration for putting messages. 
*/

#include <string.h>
#include "cphRequester.h"

static CPH_RUN cphRequesterRun( void * lpParam );
static int cphOneInteration(CPH_Requester *pRequester);




/*
** Method: cphRequesterIni
**
** Create and initialise the control block for a Requester. The worker thread control structure
** is passed in as in input parameter and is stored within the Sender - which is as close as we
** can get to creating a class derived from the worker thread.
**
** Input Parameters: pWorkerThread - the worker thread control structure
**
** Output parameters: ppRequester - a double pointer to the newly created Sender control structure
**
*/
void cphRequesterIni(CPH_Requester **ppRequester, CPH_WORKERTHREAD *pWorkerThread) {
    CPH_Requester *pRequester = NULL;
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

    char i_queue[MQ_Q_NAME_LENGTH];
    char o_queue[MQ_Q_NAME_LENGTH];
	
    int timeout;
	
    MQGMO   gmo = {MQGMO_DEFAULT};   /* get message options           */
    MQBYTE *getbuffer;
	
    int status = CPHTRUE;
	
    CPH_TRACE *pTrc = cphWorkerThreadGetTrace(pWorkerThread);
    pConfig = pWorkerThread->pConfig;
	
#define CPH_ROUTINE "cphRequesterIni"
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

    if ( (CPHTRUE == status) && (CPHTRUE != cphConfigGetString(pConfig, i_queue, "iq"))) {
      cphConfigInvalidParameter(pConfig, "(iq) In-Queue cannot be retrieved");
      status = CPHFALSE;
    }
    if ( (CPHTRUE == status) && (CPHTRUE != cphConfigGetString(pConfig, o_queue, "oq"))) {
      cphConfigInvalidParameter(pConfig, "(oq) Out-Queue cannot be retrieved");
      status = CPHFALSE;
    }
	
    if ( (CPHTRUE == status) &&
		(NULL == (pRequester = malloc(sizeof(CPH_Requester)))) ) {
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
	if (CPHTRUE == pRequester->useRFH2) gmo.Options |= MQGMO_PROPERTIES_FORCE_MQRFH2;
	
	gmo.WaitInterval = timeout * 1000;   /* Set the timeout value in ms   */

	/* Generate a unique CorrelId for this instance, in this case the address of the structure itself */

	memset(&pRequester->CorrelId,0,sizeof(MQBYTE24));
	sprintf( (char *) &pRequester->CorrelId,"%lx",(long unsigned int)&pRequester->CorrelId);
	
	/***************************************************************************/
	/* Ensure a new MsgId is created for each message but not a new CorrelId.  */
	/***************************************************************************/
	
	pmo.Options |= MQPMO_NEW_MSG_ID;
	
	if (CPHTRUE == pConnection->isTransacted) pmo.Options |= MQPMO_SYNCPOINT;
	
	if (CPHTRUE == status) {
		pRequester->pConnection = pConnection;      
		pRequester->pWorkerThread = pWorkerThread;
		pRequester->pWorkerThread->pRunFn = cphRequesterRun;
		pRequester->pWorkerThread->pOneIterationFn = (int (*) (void *) ) cphOneInteration;
		pRequester->put.buffer = putbuffer;
		pRequester->put.messLen = messLen + (MQLONG)rfh2Len;
		pRequester->put.isPersistent = isPersistent;
		pRequester->get.isReadAhead = isReadAhead;
		pRequester->useRFH2 = useRFH2;
		pRequester->useMessageHandle = useMessageHandle;
		pRequester->put.Hmsg = MQHM_NONE;
		pRequester->pTrc = pTrc;
		memcpy(&(pRequester->put.md), &md, sizeof(MQMD));
		pmo.Version = MQPMO_VERSION_3;
		memcpy(&(pRequester->put.pmo), &pmo, sizeof(MQPMO));
		
		strncpy(pRequester->i_queue, i_queue, MQ_Q_NAME_LENGTH);
		strncpy(pRequester->o_queue, o_queue, MQ_Q_NAME_LENGTH);
		strncpy(pRequester->put.md.ReplyToQ, o_queue, MQ_Q_NAME_LENGTH);
		
		pRequester->get.Hmsg = MQHM_NONE;
		pRequester->get.timeout = timeout;
		pRequester->get.buffer = getbuffer;
		pRequester->get.bufflen = messLen + (MQLONG)rfh2Len;
		
		memcpy(&(pRequester->get.gmo), &gmo, sizeof(MQGMO));
		
		/* Set the thread name according to our prefix */
		cphWorkerThreadSetThreadName(pWorkerThread, CPH_Requester_THREADNAME_PREFIX);
		*ppRequester = pRequester;
	}

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE
}


/*
** Method: cphRequesterFree
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
int cphRequesterFree(CPH_Requester **ppRequester) {
    int res = CPHFALSE;
    CPH_TRACE *pTrc;

    /* Take a local copy of pTrc so we can call CPHTRACEEXIT after we have freed the Sender */
    pTrc = (*ppRequester)->pTrc;
	
    #define CPH_ROUTINE "cphRequesterFree"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

	if (NULL != *ppRequester) {
        if (NULL != (*ppRequester)->pConnection)
			cphConnectionFree(&(*ppRequester)->pConnection);
        
		if (NULL != (*ppRequester)->put.buffer)
            free((*ppRequester)->put.buffer); 
        
		if (NULL != (*ppRequester)->get.buffer) 
            free((*ppRequester)->get.buffer); 

        if (NULL != (*ppRequester)->pWorkerThread)
			cphWorkerThreadFree( &(*ppRequester)->pWorkerThread );
        
		free(*ppRequester);
		*ppRequester = NULL;
		res = CPHTRUE;
	}
    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE
	return(res);
}


/*
** Method: cphRequesterRun
**
** This is the main execution method for the Sender thread. It makes the connection to MQ, 
** opens the destination and then calls the worker thread pace method. The latter is responsible for 
** calling the cphOneInteration method at the correct rate according to the command line
** parameters. Once the pace method exits, cphRequesterRun closes the connection to MQ and backs 
** out any messages put (if running transactionally) that were not already committed.
**
** Input Parameters: lpParam - a void * pointer to the Sender control structure. This has to be passed in as 
** a void pointer as a requirement of the thread creation routines.
**
** Returns: On windows - CPHTRUE on successful execution, CPHFALSE otherwise
**          On Linux   - NULL
**
*/
static CPH_RUN cphRequesterRun( void * lpParam ) {

    CPH_Requester *pRequester;
    CPH_WORKERTHREAD * pWorker;
    CPH_LOG *pLog;

    /*   Declare MQI structures needed                                */
    MQOD     i_od = {MQOD_DEFAULT};    /* Object Descriptor for in (put) queue */
    MQOD     o_od = {MQOD_DEFAULT};    /* Object Descriptor for out (get) queue */

    MQLONG   O_options;              /* MQOPEN options                */ 
    MQLONG   C_options;              /* MQCLOSE options               */
    
    MQLONG   CompCode;               /* completion code               */
    MQLONG   i_OpenCode;             /* in-queue MQOPEN completion code */
    MQLONG   o_OpenCode;             /* out-q MQOPEN completion code  */
    MQLONG   Reason;                 /* reason code                   */
    MQLONG   CReason;                /* reason code for MQCONN        */

    MQCNO ConnectOpts = {MQCNO_DEFAULT};

    MQCD     ClientConn = {MQCD_CLIENT_CONN_DEFAULT};
                                    /* Client connection channel     */
                                    /* definition                    */

    MQDMHO   DltMH_options = {MQDMHO_DEFAULT};

    int iterations;

	pRequester = lpParam;
	pWorker = pRequester->pWorkerThread;
    pLog = pWorker->pConfig->pLog;

    #define CPH_ROUTINE "cphRequesterRun"
    CPHTRACEENTRY(pRequester->pTrc, CPH_ROUTINE );

	sprintf(pRequester->messageString, "%s%s", cphWorkerThreadGetName(pWorker), " : START");
    cphLogPrintLn(pLog, LOGINFO, pRequester->messageString);

    cphConnectionSetConnectOpts(pRequester->pConnection, &ClientConn, &ConnectOpts);

	sprintf(pRequester->messageString, "Connect to QM %s", pRequester->pConnection->QMName);
    cphLogPrintLn(pLog, LOGINFO, pRequester->messageString);

    MQCONNX(pRequester->pConnection->QMName,     /* queue manager                  */
          &ConnectOpts,
          &(pRequester->Hcon),     /* connection handle              */
          &CompCode,               /* completion code                */
          &CReason);               /* reason code                    */


    /* report reason and stop if it failed     */
    if (CompCode == MQCC_FAILED) {
      sprintf(pRequester->messageString, "MQCONNX ended with reason code %d", CReason);
      cphLogPrintLn(pLog, LOGERROR, pRequester->messageString );
      exit( (int)CReason );
    }

   /******************************************************************/
   /*                                                                */
   /*   Open the target queues for output                            */
   /*                                                                */
   /******************************************************************/
   
       //strncpy(od.ObjectName, pRequester->topic, MQ_TOPIC_NAME_LENGTH);

   strncpy(i_od.ObjectName, pRequester->i_queue, 48);
   strncpy(o_od.ObjectName, pRequester->o_queue, 48);

   strncpy(i_od.ObjectQMgrName, pRequester->pConnection->QMName, 48);
   strncpy(o_od.ObjectQMgrName, pRequester->pConnection->QMName, 48);

   i_od.ObjectType = MQOT_Q;
   i_od.Version = MQOD_VERSION_4;

   o_od.ObjectType = MQOT_Q;
   o_od.Version = MQOD_VERSION_4;


   /* TODO: do we want a different set of O_options for the in & out queues? */
   O_options =  MQOO_OUTPUT            /* open queue for output     */
              | MQOO_INPUT_SHARED      /* Receivers in parallel       */
              | MQOO_FAIL_IF_QUIESCING; /* but not if MQM stopping   */
   if (CPHTRUE == pRequester->get.isReadAhead) O_options |= MQOO_READ_AHEAD; /* Read Ahead */
    
	
    sprintf(pRequester->messageString, "Open in-queue %s", pRequester->i_queue);
    cphLogPrintLn(pLog, LOGINFO, pRequester->messageString);
    sprintf(pRequester->messageString, "Open out-queue %s", pRequester->o_queue);
    cphLogPrintLn(pLog, LOGINFO, pRequester->messageString);

   /**********************/
   /* open the in-queue  */
   /**********************/
   MQOPEN(pRequester->Hcon,                      /* connection handle            */
          &i_od,                       /* object descriptor for queue  */
          O_options,                 /* open options                 */
          &pRequester->i_Hobj,      /* object handle                */
          &i_OpenCode,                 /* MQOPEN completion code       */
          &Reason);                  /* reason code                  */

   /* report reason, if any; stop if failed      */
   if (Reason != MQRC_NONE)
   {
      sprintf(pRequester->messageString, "MQOPEN ended with reason code %d", Reason);
      cphLogPrintLn(pLog, LOGERROR, pRequester->messageString );
   }

   if (i_OpenCode == MQCC_FAILED)
   {
      cphLogPrintLn(pLog, LOGERROR, "unable to open queue for output" );
      exit( (int)Reason );
   }

   /**********************/
   /* open the out-queue */
   /**********************/
   MQOPEN(pRequester->Hcon,                      /* connection handle            */
          &o_od,                       /* object descriptor for queue  */
          O_options,                 /* open options                 */
          &pRequester->o_Hobj,      /* object handle                */
          &o_OpenCode,                 /* MQOPEN completion code       */
          &Reason);                  /* reason code                  */

   /* report reason, if any; stop if failed      */
   if (Reason != MQRC_NONE)
   {
      sprintf(pRequester->messageString, "MQOPEN ended with reason code %d", Reason);
      cphLogPrintLn(pLog, LOGERROR, pRequester->messageString );
   }

   if (o_OpenCode == MQCC_FAILED)
   {
      cphLogPrintLn(pLog, LOGERROR, "unable to open queue for output" );
      exit( (int)Reason );
   }


    cphWorkerThreadSetState(pWorker, sRUNNING);
	
    sprintf(pRequester->messageString, "%s%s", cphWorkerThreadGetName(pWorker), " : Entering client loop");
    cphLogPrintLn(pWorker->pConfig->pLog, LOGVERBOSE, pRequester->messageString);

    cphWorkerThreadPace(pWorker, pRequester);
 
    iterations = cphWorkerThreadGetIterations(pWorker);
    if ( (CPHTRUE == pRequester->pConnection->isTransacted) && (iterations != 0) && (iterations%pRequester->pConnection->commitCount!=0) ) {
        CPHTRACEMSG(pRequester->pTrc, CPH_ROUTINE, "Calling MQBACK");
        MQBACK(pRequester->Hcon, &CompCode, &Reason);
    }

    if (pRequester->put.Hmsg != MQHM_NONE)
    {
      MQDLTMH(pRequester->Hcon, &pRequester->put.Hmsg, &DltMH_options, &CompCode, &Reason);
    }

   /******************************************************************/
   /*                                                                */
   /*   Close the target queues (if they were opened)                */
   /*                                                                */
   /******************************************************************/
   if (i_OpenCode != MQCC_FAILED)
   {
       C_options = MQCO_NONE;        /* no close options             */

       MQCLOSE(pRequester->Hcon,    /* connection handle            */
             &pRequester->i_Hobj,   /* object handle                */
             C_options,
             &CompCode,              /* completion code              */
             &Reason);               /* reason code                  */

     /* report reason, if any     */
     if (Reason != MQRC_NONE)
     {
      sprintf(pRequester->messageString, "MQCLOSE ended with reason code %d", Reason);
      cphLogPrintLn(pLog, LOGERROR, pRequester->messageString );
     }
   }
   if (o_OpenCode != MQCC_FAILED)
   {
       C_options = MQCO_NONE;        /* no close options             */

       MQCLOSE(pRequester->Hcon,    /* connection handle            */
             &pRequester->o_Hobj,   /* object handle                */
             C_options,
             &CompCode,              /* completion code              */
             &Reason);               /* reason code                  */

     /* report reason, if any     */
     if (Reason != MQRC_NONE)
     {
      sprintf(pRequester->messageString, "MQCLOSE ended with reason code %d", Reason);
      cphLogPrintLn(pLog, LOGERROR, pRequester->messageString );
     }
   }


    /* Disconnect from the queue manager */
    if (CReason != MQRC_ALREADY_CONNECTED)  {
       MQDISC(&pRequester->Hcon,                   /* connection handle            */
              &CompCode,               /* completion code              */
              &Reason);                /* reason code                  */

       /* report reason, if any     */
       if (Reason != MQRC_NONE) {
          sprintf(pRequester->messageString, "MQDISC ended with reason code %d", Reason);
          cphLogPrintLn(pLog, LOGERROR, pRequester->messageString );
       }
    }

    sprintf(pRequester->messageString, "%s%s", cphWorkerThreadGetName(pWorker), " : STOP");
    cphLogPrintLn(pWorker->pConfig->pLog, LOGINFO, pRequester->messageString);

    //state = (state&sERROR)|sENDED;
    cphWorkerThreadSetState(pWorker, (cphWorkerThreadGetState(pWorker) & sERROR) | sENDED);

    CPHTRACEEXIT(pRequester->pTrc, CPH_ROUTINE);
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

static int cphPutOneInteration(CPH_Requester *pRequester) {
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
    CPHTRACEENTRY(pRequester->pTrc, CPH_ROUTINE );

    CPHTRACEMSG(pRequester->pTrc, CPH_ROUTINE, "Message to be sent: %s", pRequester->put.buffer);
    /*printf("cphPutOneIteration: REQUESTER\n");*/

    if ((pRequester->useMessageHandle) && (pRequester->put.Hmsg == MQHM_NONE))
    {
      MQCRTMH(pRequester->Hcon,
              &CrtMHOpts,
              &pRequester->put.Hmsg,
              &CompCode,
              &Reason);
      if (Reason != MQRC_NONE)
      {
        sprintf(pRequester->messageString, "MQCRTMH ended with reason code %d", Reason);
        cphLogPrintLn(pRequester->pWorkerThread->pConfig->pLog, LOGERROR, pRequester->messageString );
        status = CPHFALSE;
      }

      if (status == CPHTRUE)
      {
        Rfh2Buf = cphBuildRFH2(&Rfh2Len);
        memcpy(pRequester->put.md.Format, MQFMT_RF_HEADER_2, (size_t)MQ_FORMAT_LENGTH);

        MQBUFMH(pRequester->Hcon, pRequester->put.Hmsg, &BufMHOpts, &pRequester->put.md, Rfh2Len, Rfh2Buf, &DataLen, &CompCode, &Reason);

        /* report reason, if any */
        if (Reason != MQRC_NONE) {
             sprintf(pRequester->messageString, "MQBUFMH ended with reason code %d\n", Reason);
             cphLogPrintLn(pRequester->pWorkerThread->pConfig->pLog, LOGERROR, pRequester->messageString );
             status = CPHFALSE;
        }

        free(Rfh2Buf);
      }
    }
      else
    {
       buffer2 = malloc(pRequester->put.messLen);
       memcpy(buffer2, pRequester->put.buffer, pRequester->put.messLen);
       userfh2 = CPHTRUE;
    }

    if (status == CPHTRUE)
    {

      memcpy(pRequester->put.md.CorrelId, pRequester->CorrelId, sizeof(MQBYTE24));  
      pRequester->put.pmo.OriginalMsgHandle = pRequester->put.Hmsg;
      MQPUT(pRequester->Hcon,       /* connection handle               */
             pRequester->i_Hobj,      /* object handle                   */
             &pRequester->put.md,     /* message descriptor              */
             &pRequester->put.pmo,    /* default options (datagram)      */
             pRequester->put.messLen, /* message length                  */
			 (userfh2 ? buffer2 : pRequester->put.buffer),  /* message buffer                  */
             &CompCode,           /* completion code                 */
             &Reason);            /* reason code                     */
    /* report reason, if any */
    if (Reason != MQRC_NONE) {
         sprintf(pRequester->messageString, "MQPUT ended with reason code %d\n", Reason);
         cphLogPrintLn(pRequester->pWorkerThread->pConfig->pLog, LOGERROR, pRequester->messageString );
         status = CPHFALSE;
    } else {
        if ( (CPHTRUE == pRequester->pConnection->isTransacted) && ((cphWorkerThreadGetIterations(pRequester->pWorkerThread)+1)%pRequester->pConnection->commitCount==0) ) {
            CPHTRACEMSG(pRequester->pTrc, CPH_ROUTINE, "Calling MQCMIT");
            MQCMIT(pRequester->Hcon, &CompCode, &Reason);
        }
    }
    }
    if (userfh2)
		free(buffer2);

    CPHTRACEEXIT(pRequester->pTrc, CPH_ROUTINE);
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
static int cphGetOneInteration(CPH_Requester *pRequester) {
    CPH_WORKERTHREAD *pWorkerThread;
    CPH_LOG *pLog;
    int iterations;
    int status = CPHTRUE;

    MQMD     md = {MQMD_DEFAULT};    /* Message Descriptor            */

    MQLONG   CompCode;               /* completion code               */
    MQLONG   Reason;                 /* reason code                   */
    MQLONG   messlen;                /* message length received       */

    pWorkerThread = pRequester->pWorkerThread;
    pLog = pWorkerThread->pConfig->pLog;

#define CPH_ROUTINE "cphOneInteration"
    CPHTRACEENTRY(pRequester->pTrc, CPH_ROUTINE );

    /*printf("cphGetOneIteration: REQUESTER\n");*/
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
    memcpy(md.CorrelId, pRequester->CorrelId, sizeof(md.CorrelId));


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
    CPHTRACEMSG(pRequester->pTrc, CPH_ROUTINE, "Calling MQGET : %d seconds wait time", pRequester->get.gmo.WaitInterval / 1000);

    MQGET(pRequester->Hcon, /* connection handle                 */
        pRequester->o_Hobj,   /* object handle                     */
        &md,                 /* message descriptor                */
        &pRequester->get.gmo,   /* get message options               */
        pRequester->get.bufflen,/* buffer length                     */
        pRequester->get.buffer, /* message buffer                    */
        &messlen,            /* message length                    */
        &CompCode,           /* completion code                   */
        &Reason);            /* reason code                       */

    /* report reason, if any     */
    if (Reason != MQRC_NONE)
    {
        if (Reason == MQRC_TRUNCATED_MSG_ACCEPTED) {
            /* This test uses a matched get buffer size so log a message incase this is important */
            sprintf(pRequester->messageString, "Received message size[%d] exceeds buffer size[%d]. Growing buffer.", messlen, pRequester->get.bufflen);
            cphLogPrintLn(pLog, LOGERROR, pRequester->messageString);

            CPHTRACEMSG(pRequester->pTrc, CPH_ROUTINE, "Msg truncated. Extending buffer to %d bytes", messlen);
            pRequester->get.buffer = realloc( pRequester->get.buffer, (size_t) messlen);
            if (NULL == pRequester->get.buffer) {
                sprintf(pRequester->messageString, "ERROR - cannot extend buffer to %d bytes.", messlen);
                cphLogPrintLn(pLog, LOGERROR, pRequester->messageString);
                status = CPHFALSE;
            }
            else
                pRequester->get.bufflen = messlen;
        } else if (Reason == MQRC_NO_MSG_AVAILABLE) {
            /* special report for normal end    */
            CPHTRACEMSG(pRequester->pTrc, "%s", "no message received in wait interval");
            status = CPH_RXTIMEDOUT;
        } else {
            /* general report for other reasons */
            sprintf(pRequester->messageString, "MQGET ended with reason code %d", Reason);
            cphLogPrintLn(pLog, LOGERROR, pRequester->messageString);
            status = CPHFALSE;
        }
    } else {
        iterations = cphWorkerThreadGetIterations(pWorkerThread);
        if ( (CPHTRUE == pRequester->pConnection->isTransacted) && ((iterations+1)%pRequester->pConnection->commitCount==0) ) {
            CPHTRACEMSG(pRequester->pTrc, CPH_ROUTINE, "Calling MQCMIT");
            MQCMIT(pRequester->Hcon, &CompCode, &Reason);
        }

    }

    CPHTRACEEXIT(pRequester->pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE
    return(status);
}

static int cphOneInteration(CPH_Requester *pRequester) {
    int status = cphPutOneInteration(pRequester);

    /*printf("cphOneIteration: REQUESTER: %d\n", status);*/

    if (status == CPHTRUE)
    {
        status = cphGetOneInteration(pRequester);
    }
    return status;
}



