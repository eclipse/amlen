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

/* This module is the implementation of the "Responder" test suite. This provides a set of functions that are used to put messages under
** the control of the worker thread function, which controls the rate and duration for putting messages. 
*/

#include <string.h>
#include "cphResponder.h"

static CPH_RUN cphResponderRun( void * lpParam );
static int cphOneInteration(CPH_Responder *pResponder);




/*
** Method: cphResponderIni
**
** Create and initialise the control block for a Responder. The worker thread control structure
** is passed in as in input parameter and is stored within the Sender - which is as close as we
** can get to creating a class derived from the worker thread.
**
** Input Parameters: pWorkerThread - the worker thread control structure
**
** Output parameters: ppResponder - a double pointer to the newly created Sender control structure
**
*/
void cphResponderIni(CPH_Responder **ppResponder, CPH_WORKERTHREAD *pWorkerThread) {
    CPH_Responder *pResponder = NULL;
    CPH_CONFIG *pConfig;
    int messLen;
    int isPersistent;
    int isReadAhead;
    int useRFH2;
    size_t rfh2Len = 0;
    int useMessageHandle;
    char *putbuffer;
    int cr;
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
	
#define CPH_ROUTINE "cphResponderIni"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );
	
    printf("cphResponderIni: ENTRY\n");

	if (NULL == pWorkerThread) {
		CPHTRACEEXIT(pTrc, CPH_ROUTINE);
		return;
	}
	
    cphConnectionIni(&pConnection, pConfig);   
    printf("cphResponderIni: HERE4\n");
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
    printf("cphResponderIni: HERE3\n");
	
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

    printf("cphResponderIni: HERE5\n");
    if ( (CPHTRUE == status) && (CPHTRUE != cphConfigGetString(pConfig, i_queue, "iq"))) {
      cphConfigInvalidParameter(pConfig, "(iq) In-Queue cannot be retrieved");
      status = CPHFALSE;
    }
    if ( (CPHTRUE == status) && (CPHTRUE != cphConfigGetString(pConfig, o_queue, "oq"))) {
      cphConfigInvalidParameter(pConfig, "(oq) Out-Queue cannot be retrieved");
      status = CPHFALSE;
    }
    if ( (CPHTRUE == status) && (CPHTRUE != cphConfigGetBoolean(pConfig, &cr, "cr"))) {
      cphConfigInvalidParameter(pConfig, "Cannot retrieve copy request option (cr)");
      status = CPHFALSE;
    } else {
	CPHTRACEMSG(pTrc, CPH_ROUTINE, "copyRequestIntoResponse: %d.", cr);
    }
	
    printf("cphResponderIni: HERE1\n");
	
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
	
    printf("cphResponderIni: HERE2\n");
	if ( (CPHTRUE == status) && (CPHTRUE != cphConfigGetInt(pConfig, &timeout, "to"))) {
		cphConfigInvalidParameter(pConfig, "Timeout value cannot be retrieved");
		status = CPHFALSE;
	}
	else {
		CPHTRACEMSG(pTrc, CPH_ROUTINE, "MQGET Timeout: %d.", timeout);
	}
	
    printf("cphResponderIni: ABOUT TO CALL cphConnectionParseArgs\n");
    if ( (CPHTRUE == status) && (CPHTRUE != cphConnectionParseArgs(pConnection, pWorkerThread->pControlThread->pDestinationFactory))) {
		cphConfigInvalidParameter(pConfig, "Cannot parse options for MQ connection");
		status = CPHFALSE;
		
    }
	
	if ( (CPHTRUE == status) && (NULL == (getbuffer = malloc(messLen + (MQLONG)rfh2Len)))) {
		cphConfigInvalidParameter(pConfig, "Cannot allocate memory for message buffer");
		status = CPHFALSE;
	}
	
    if ( (CPHTRUE == status) &&
		(NULL == (pResponder = malloc(sizeof(CPH_Responder)))) ) {
		cphConfigInvalidParameter(pConfig, "Cannot allocate memory for Sender control block");
		status = CPHFALSE;
	} 
	
	if (cr) {
        	pResponder->put.buffer = (char*)pResponder->get.buffer; /* join pointers */
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
	if (CPHTRUE == useRFH2) gmo.Options |= MQGMO_PROPERTIES_FORCE_MQRFH2;
	
	gmo.WaitInterval = timeout * 1000;   /* Set the timeout value in ms   */

	/* Generate a unique CorrelId for this instance, in this case the address of the structure itself */

	memset(&pResponder->CorrelId,0,sizeof(MQBYTE24));
	sprintf( (char *) &pResponder->CorrelId,"%lx",(long unsigned int)&pResponder->CorrelId);
	
	/***************************************************************************/
	/* Ensure a new MsgId is created for each message but not a new CorrelId.  */
	/***************************************************************************/
	
	pmo.Options |= MQPMO_NEW_MSG_ID;
	
	if (CPHTRUE == pConnection->isTransacted) pmo.Options |= MQPMO_SYNCPOINT;
	
	if (CPHTRUE == status) {
		pResponder->pConnection = pConnection;      
		pResponder->pWorkerThread = pWorkerThread;
		pResponder->pWorkerThread->pRunFn = cphResponderRun;
		pResponder->pWorkerThread->pOneIterationFn = (int (*) (void *) ) cphOneInteration;
		pResponder->put.buffer = putbuffer;
		pResponder->put.messLen = messLen + (MQLONG)rfh2Len;
		pResponder->put.isPersistent = isPersistent;
		pResponder->get.isReadAhead = isReadAhead;
		pResponder->useRFH2 = useRFH2;
		pResponder->useMessageHandle = useMessageHandle;
                pResponder->put.cr = cr;
		pResponder->put.Hmsg = MQHM_NONE;
		pResponder->pTrc = pTrc;
		memcpy(&(pResponder->put.md), &md, sizeof(MQMD));
		pmo.Version = MQPMO_VERSION_3;
		memcpy(&(pResponder->put.pmo), &pmo, sizeof(MQPMO));
		
		strncpy(pResponder->i_queue, i_queue, MQ_Q_NAME_LENGTH);
		strncpy(pResponder->o_queue, o_queue, MQ_Q_NAME_LENGTH);
		
		pResponder->get.Hmsg = MQHM_NONE;
		pResponder->get.timeout = timeout;
		pResponder->get.buffer = getbuffer;
		pResponder->get.bufflen = messLen + (MQLONG)rfh2Len;
		
		memcpy(&(pResponder->get.gmo), &gmo, sizeof(MQGMO));
		
		/* Set the thread name according to our prefix */
		cphWorkerThreadSetThreadName(pWorkerThread, CPH_Responder_THREADNAME_PREFIX);
		*ppResponder = pResponder;
	}

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE
}


/*
** Method: cphResponderFree
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
int cphResponderFree(CPH_Responder **ppResponder) {
    int res = CPHFALSE;
    CPH_TRACE *pTrc;

    /* Take a local copy of pTrc so we can call CPHTRACEEXIT after we have freed the Sender */
    pTrc = (*ppResponder)->pTrc;
	
    #define CPH_ROUTINE "cphResponderFree"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

	if (NULL != *ppResponder) {
        if (NULL != (*ppResponder)->pConnection)
		cphConnectionFree(&(*ppResponder)->pConnection);
        
/*
        if (NULL != (*ppResponder)->get.od)
            free((*ppResponder)->get.od);

        if (NULL != (*ppResponder)->put.od)
            free((*ppResponder)->put.od);
*/

	if (!(*ppResponder)->put.cr) {
	  if (NULL != (*ppResponder)->put.buffer)
              free((*ppResponder)->put.buffer); 
        }
        
	if (NULL != (*ppResponder)->get.buffer) 
            free((*ppResponder)->get.buffer); 

        if (NULL != (*ppResponder)->pWorkerThread)
		cphWorkerThreadFree( &(*ppResponder)->pWorkerThread );
        
	free(*ppResponder);
	*ppResponder = NULL;
	res = CPHTRUE;
    }
    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE
	return(res);
}


/*
** Method: cphResponderRun
**
** This is the main execution method for the Sender thread. It makes the connection to MQ, 
** opens the destination and then calls the worker thread pace method. The latter is responssible for 
** calling the cphOneInteration method at the correct rate according to the command line
** parameters. Once the pace method exits, cphResponderRun closes the connection to MQ and backs 
** out any messages put (if running transactionally) that were not already committed.
**
** Input Parameters: lpParam - a void * pointer to the Sender control structure. This has to be passed in as 
** a void pointer as a requirement of the thread creation routines.
**
** Returns: On windows - CPHTRUE on successful execution, CPHFALSE otherwise
**          On Linux   - NULL
**
*/
static CPH_RUN cphResponderRun( void * lpParam ) {

    CPH_Responder *pResponder;
    CPH_WORKERTHREAD * pWorker;
    CPH_LOG *pLog;

    /*   Declare MQI structures needed                                */
    MQOD     i_od = {MQOD_DEFAULT};    /* Object Descriptor for in (get) queue */
    MQOD     o_od = {MQOD_DEFAULT};    /* Object Descriptor for out (put) queue */

    MQLONG   CompCode;               /* completion code               */
    MQLONG   Reason;                 /* reason code                   */
    MQLONG   CReason;                /* reason code for MQCONN        */

    MQCNO ConnectOpts = {MQCNO_DEFAULT};

    MQCD     ClientConn = {MQCD_CLIENT_CONN_DEFAULT};
                                    /* Client connection channel     */
                                    /* definition                    */

    MQDMHO   DltMH_options = {MQDMHO_DEFAULT};

    int iterations;

    pResponder = lpParam;
    pWorker = pResponder->pWorkerThread;
    pLog = pWorker->pConfig->pLog;

    #define CPH_ROUTINE "cphResponderRun"
    CPHTRACEENTRY(pResponder->pTrc, CPH_ROUTINE );

    sprintf(pResponder->messageString, "%s%s", cphWorkerThreadGetName(pWorker), " : START");
    cphLogPrintLn(pLog, LOGINFO, pResponder->messageString);

    cphConnectionSetConnectOpts(pResponder->pConnection, &ClientConn, &ConnectOpts);

    sprintf(pResponder->messageString, "Connect to QM %s", pResponder->pConnection->QMName);
    cphLogPrintLn(pLog, LOGINFO, pResponder->messageString);

    MQCONNX(pResponder->pConnection->QMName,     /* queue manager                  */
          &ConnectOpts,
          &(pResponder->Hcon),     /* connection handle              */
          &CompCode,               /* completion code                */
          &CReason);               /* reason code                    */


    /* report reason and stop if it failed     */
    if (CompCode == MQCC_FAILED) {
      sprintf(pResponder->messageString, "MQCONNX ended with reason code %d", CReason);
      cphLogPrintLn(pLog, LOGERROR, pResponder->messageString );
      exit( (int)CReason );
    }

   /******************************************************************/
   /*                                                                */
   /*   Open the target queues                                       */
   /*                                                                */
   /******************************************************************/
   
   pResponder->get.od = &i_od;
   pResponder->put.od = &o_od;

   strncpy(pResponder->get.od->ObjectName, pResponder->i_queue, 48);
   /* NOTE: Mustn't set put ObjectName here or the OpenPutQueue fn will fail */

   /* set to fail so we don't try & close on first instantiation */
   pResponder->put.OpenCode = MQCC_FAILED;

   strncpy(pResponder->put.od->ObjectQMgrName, pResponder->pConnection->QMName, 48);
   strncpy(pResponder->get.od->ObjectQMgrName, pResponder->pConnection->QMName, 48);

   pResponder->get.od->ObjectType = MQOT_Q;
   pResponder->get.od->Version = MQOD_VERSION_4;
   pResponder->put.od->ObjectType = MQOT_Q;
   pResponder->put.od->Version = MQOD_VERSION_4;

   /* TODO: do we want a different set of O_options for the in & out queues? */
   pResponder->O_options =  MQOO_OUTPUT   /* open queue for output     */
              | MQOO_INPUT_SHARED         /* Receivers in parallel       */
              | MQOO_FAIL_IF_QUIESCING;   /* but not if MQM stopping   */

   if (CPHTRUE == pResponder->get.isReadAhead) pResponder->O_options |= MQOO_READ_AHEAD; /* Read Ahead */
    
   pResponder->C_options = MQCO_NONE;        /* no close options             */
	
   sprintf(pResponder->messageString, "Open in-queue %s", pResponder->i_queue);
   cphLogPrintLn(pLog, LOGINFO, pResponder->messageString);

   /**********************/
   /* open the in-queue  */
   /**********************/
   MQOPEN(pResponder->Hcon,                      /* connection handle            */
          pResponder->get.od,                   /* object descriptor for queue  */
          pResponder->O_options,                 /* open options                 */
          &pResponder->get.Hobj,      /* object handle                */
          &pResponder->get.OpenCode,                 /* MQOPEN completion code       */
          &Reason);                  /* reason code                  */

   /* report reason, if any; stop if failed      */
   if (Reason != MQRC_NONE)
   {
      sprintf(pResponder->messageString, "MQOPEN ended with reason code %d", Reason);
      cphLogPrintLn(pLog, LOGERROR, pResponder->messageString );
   }

   if (pResponder->get.OpenCode == MQCC_FAILED)
   {
      cphLogPrintLn(pLog, LOGERROR, "unable to open queue for input" );
      exit( (int)Reason );
   }



    cphWorkerThreadSetState(pWorker, sRUNNING);
	
    sprintf(pResponder->messageString, "%s%s", cphWorkerThreadGetName(pWorker), " : Entering client loop");
    cphLogPrintLn(pWorker->pConfig->pLog, LOGVERBOSE, pResponder->messageString);

    cphWorkerThreadPace(pWorker, pResponder);
 
    iterations = cphWorkerThreadGetIterations(pWorker);
    if ( (CPHTRUE == pResponder->pConnection->isTransacted) && (iterations != 0) && (iterations%pResponder->pConnection->commitCount!=0) ) {
        CPHTRACEMSG(pResponder->pTrc, CPH_ROUTINE, "Calling MQBACK");
        MQBACK(pResponder->Hcon, &CompCode, &Reason);
    }

    if (pResponder->put.Hmsg != MQHM_NONE)
    {
      MQDLTMH(pResponder->Hcon, &pResponder->put.Hmsg, &DltMH_options, &CompCode, &Reason);
    }

   /******************************************************************/
   /*                                                                */
   /*   Close the target queues (if they were opened)                */
   /*                                                                */
   /******************************************************************/
   if (pResponder->get.OpenCode != MQCC_FAILED)
   {
       MQCLOSE(pResponder->Hcon,    /* connection handle            */
             &pResponder->get.Hobj,   /* object handle                */
             pResponder->C_options,
             &CompCode,              /* completion code              */
             &Reason);               /* reason code                  */

     /* report reason, if any     */
     if (Reason != MQRC_NONE)
     {
      sprintf(pResponder->messageString, "MQCLOSE ended with reason code %d", Reason);
      cphLogPrintLn(pLog, LOGERROR, pResponder->messageString );
     }
   }
   if (pResponder->put.OpenCode != MQCC_FAILED)
   {
       MQCLOSE(pResponder->Hcon,    /* connection handle            */
             &pResponder->put.Hobj,   /* object handle                */
             pResponder->C_options,
             &CompCode,              /* completion code              */
             &Reason);               /* reason code                  */

     /* report reason, if any     */
     if (Reason != MQRC_NONE)
     {
      sprintf(pResponder->messageString, "MQCLOSE ended with reason code %d", Reason);
      cphLogPrintLn(pLog, LOGERROR, pResponder->messageString );
     }
   }


    /* Disconnect from the queue manager */
    if (CReason != MQRC_ALREADY_CONNECTED)  {
       MQDISC(&pResponder->Hcon,                   /* connection handle            */
              &CompCode,               /* completion code              */
              &Reason);                /* reason code                  */

       /* report reason, if any     */
       if (Reason != MQRC_NONE) {
          sprintf(pResponder->messageString, "MQDISC ended with reason code %d", Reason);
          cphLogPrintLn(pLog, LOGERROR, pResponder->messageString );
       }
    }

	sprintf(pResponder->messageString, "%s%s", cphWorkerThreadGetName(pWorker), " : STOP");
    cphLogPrintLn(pWorker->pConfig->pLog, LOGINFO, pResponder->messageString);

    //state = (state&sERROR)|sENDED;
    cphWorkerThreadSetState(pWorker, (cphWorkerThreadGetState(pWorker) & sERROR) | sENDED);

    CPHTRACEEXIT(pResponder->pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE

    /* Return the right type of variable according to the o/s. (This return is not used) */
    return CPH_RUNRETURN;
}


/* set pResponder->o_queue to whatever queue we want to open */
/* if it's already open, return                              */
/* if another queue is already open, close it & open this one*/
static int cphResponderOpenPutQueue(CPH_Responder *pResponder) {
    MQLONG   CompCode;             /* MQOPEN completion code  */
    MQLONG   Reason;               /* reason code             */

    /*printf("cphResponderOpenPutQueue: %s\n", pResponder->o_queue);*/

    /************************************************************/
    /* is there a queue already open with the put object handle */
    /* is it the queue we want?                                 */
    /************************************************************/
    if (strcmp(pResponder->put.od->ObjectName, pResponder->o_queue) == 0) {
      return 0; 
    } else {
      /* close */
      if (pResponder->put.OpenCode != MQCC_FAILED)
      {
          pResponder->C_options = MQCO_NONE;        /* no close options             */

          MQCLOSE(pResponder->Hcon,    /* connection handle            */
                  &pResponder->put.Hobj,   /* object handle                */
                  pResponder->C_options,
                  &CompCode,              /* completion code              */
                  &Reason);               /* reason code                  */

          /* report reason, if any     */
          if (Reason != MQRC_NONE)
          {
              sprintf(pResponder->messageString, "MQCLOSE ended with reason code %d", Reason);
              cphLogPrintLn(pResponder->pWorkerThread->pConfig->pLog, LOGERROR, pResponder->messageString );
              exit( (int)Reason );
          }
      }
    }

   strncpy(pResponder->put.od->ObjectName, pResponder->o_queue, 48);

   /******************/
   /* open the queue */
   /******************/
   MQOPEN(pResponder->Hcon,                      /* connection handle            */
           pResponder->put.od,                       /* object descriptor for queue  */
           pResponder->O_options,                 /* open options                 */
           &pResponder->put.Hobj,      /* object handle                */
           &pResponder->put.OpenCode,                 /* MQOPEN completion code       */
           &Reason);                  /* reason code                  */


   /* report reason, if any; stop if failed      */
   if (Reason != MQRC_NONE)
   {
      sprintf(pResponder->messageString, "MQOPEN ended with reason code %d", Reason);
      cphLogPrintLn(pResponder->pWorkerThread->pConfig->pLog, LOGERROR, pResponder->messageString );
   }

   if (pResponder->put.OpenCode == MQCC_FAILED)
   {
      cphLogPrintLn(pResponder->pWorkerThread->pConfig->pLog, LOGERROR, "unable to open queue for output" );
      exit( (int)Reason );
   }

   return 0;
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

static int cphPutOneInteration(CPH_Responder *pResponder) {
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
    CPHTRACEENTRY(pResponder->pTrc, CPH_ROUTINE );

    CPHTRACEMSG(pResponder->pTrc, CPH_ROUTINE, "Message to be sent: %s", pResponder->put.buffer);
    /*printf("cphputOneIteration: RESPONDER\n");*/

    if (pResponder->put.cr) {
      /* we copied the message from the getter so everything should be set */
    } else if ((pResponder->useMessageHandle) && (pResponder->put.Hmsg == MQHM_NONE))
    {
      MQCRTMH(pResponder->Hcon,
              &CrtMHOpts,
              &pResponder->put.Hmsg,
              &CompCode,
              &Reason);
      if (Reason != MQRC_NONE)
      {
        sprintf(pResponder->messageString, "MQCRTMH ended with reason code %d", Reason);
        cphLogPrintLn(pResponder->pWorkerThread->pConfig->pLog, LOGERROR, pResponder->messageString );
        status = CPHFALSE;
      }

      if (status == CPHTRUE)
      {
        Rfh2Buf = cphBuildRFH2(&Rfh2Len);
        memcpy(pResponder->put.md.Format, MQFMT_RF_HEADER_2, (size_t)MQ_FORMAT_LENGTH);

        MQBUFMH(pResponder->Hcon, pResponder->put.Hmsg, &BufMHOpts, &pResponder->put.md, Rfh2Len, Rfh2Buf, &DataLen, &CompCode, &Reason);

        /* report reason, if any */
        if (Reason != MQRC_NONE) {
             sprintf(pResponder->messageString, "MQBUFMH ended with reason code %d\n", Reason);
             cphLogPrintLn(pResponder->pWorkerThread->pConfig->pLog, LOGERROR, pResponder->messageString );
             status = CPHFALSE;
        }

        free(Rfh2Buf);
      }
    }
      else
    {
       buffer2 = malloc(pResponder->put.messLen);
       memcpy(buffer2, pResponder->put.buffer, pResponder->put.messLen);
       userfh2 = CPHTRUE;
    }

    if (status == CPHTRUE)
    {
      /* ensure we have the correct queue open */
      cphResponderOpenPutQueue(pResponder);

      pResponder->put.pmo.OriginalMsgHandle = pResponder->put.Hmsg;
      MQPUT(pResponder->Hcon,       /* connection handle               */
             pResponder->put.Hobj,      /* object handle                   */
             &pResponder->put.md,     /* message descriptor              */
             &pResponder->put.pmo,    /* default options (datagram)      */
             pResponder->put.messLen, /* message length                  */
    	     (userfh2 ? buffer2 : pResponder->put.buffer),  /* message buffer                  */
             &CompCode,           /* completion code                 */
             &Reason);            /* reason code                     */
    /* report reason, if any */
    if (Reason != MQRC_NONE) {
         sprintf(pResponder->messageString, "MQPUT ended with reason code %d\n", Reason);
         cphLogPrintLn(pResponder->pWorkerThread->pConfig->pLog, LOGERROR, pResponder->messageString );
         status = CPHFALSE;
    } else {
        if ( (CPHTRUE == pResponder->pConnection->isTransacted) && ((cphWorkerThreadGetIterations(pResponder->pWorkerThread)+1)%pResponder->pConnection->commitCount==0) ) {
            CPHTRACEMSG(pResponder->pTrc, CPH_ROUTINE, "Calling MQCMIT");
            MQCMIT(pResponder->Hcon, &CompCode, &Reason);
        }
    }
    }
    if (userfh2)
		free(buffer2);

    CPHTRACEEXIT(pResponder->pTrc, CPH_ROUTINE);
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
static int cphGetOneInteration(CPH_Responder *pResponder) {
    CPH_WORKERTHREAD *pWorkerThread;
    CPH_LOG *pLog;
    int iterations;
    int status = CPHTRUE;

    MQMD     md = {MQMD_DEFAULT};    /* Message Descriptor            */

    MQLONG   CompCode;               /* completion code               */
    MQLONG   Reason;                 /* reason code                   */
    MQLONG   messlen;                /* message length received       */

    pWorkerThread = pResponder->pWorkerThread;
    pLog = pWorkerThread->pConfig->pLog;


#define CPH_ROUTINE "cphOneInteration"
    CPHTRACEENTRY(pResponder->pTrc, CPH_ROUTINE );

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
    memcpy(md.CorrelId, MQCI_NONE, sizeof(md.CorrelId));
    /*memcpy(md.CorrelId, pResponder->CorrelId, sizeof(md.CorrelId)); */


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
    CPHTRACEMSG(pResponder->pTrc, CPH_ROUTINE, "Calling MQGET : %d seconds wait time", pResponder->get.gmo.WaitInterval / 1000);
    /*printf("cphGetOneIteration: RESPONDER : %d seconds wait time, pResponder->i_Hobj>\n", pResponder->get.gmo.WaitInterval / 1000);*/

    MQGET(pResponder->Hcon, /* connection handle                 */
        pResponder->get.Hobj,   /* object handle                     */
        &md,                 /* message descriptor                */
        &pResponder->get.gmo,   /* get message options               */
        pResponder->get.bufflen,/* buffer length                     */
        pResponder->get.buffer, /* message buffer                    */
        &messlen,            /* message length                    */
        &CompCode,           /* completion code                   */
        &Reason);            /* reason code                       */

    /* report reason, if any     */
    if (Reason != MQRC_NONE)
    {
        if (Reason == MQRC_TRUNCATED_MSG_ACCEPTED) {
            /* This test uses a matched get buffer size so log a message incase this is important */
            sprintf(pResponder->messageString, "Received message size[%d] exceeds buffer size[%d]. Growing buffer.", messlen, pResponder->get.bufflen);
            cphLogPrintLn(pLog, LOGERROR, pResponder->messageString);
            /*printf("cphGetOneIteration: RESPONDER: %s\n", pResponder->messageString);*/

            CPHTRACEMSG(pResponder->pTrc, CPH_ROUTINE, "Msg truncated. Extending buffer to %d bytes", messlen);
            pResponder->get.buffer = realloc( pResponder->get.buffer, (size_t) messlen);
            if (NULL == pResponder->get.buffer) {
                sprintf(pResponder->messageString, "ERROR - cannot extend buffer to %d bytes.", messlen);
                cphLogPrintLn(pLog, LOGERROR, pResponder->messageString);
                status = CPHFALSE;
            }
            else {
                pResponder->get.bufflen = messlen;

                /* TODO: */
                /* if we're sending back the same message, the buffer is shared */
/*
                if (pResponder->put.cr) {
                  pResponder->put.buffen = messlen;
                }
*/
            }
        } else if (Reason == MQRC_NO_MSG_AVAILABLE) {
            /* special report for normal end    */
            CPHTRACEMSG(pResponder->pTrc, "%s", "no message received in wait interval");
            /*printf("cphGetOneIteration: RESPONDER: %s\n", "no message received in wait interval");*/
            status = CPH_RXTIMEDOUT;
        } else {
            /* general report for other reasons */
            sprintf(pResponder->messageString, "MQGET ended with reason code %d", Reason);
            cphLogPrintLn(pLog, LOGERROR, pResponder->messageString);
            /*printf("cphGetOneIteration: RESPONDER: %s\n", pResponder->messageString);*/
            status = CPHFALSE;
        }
    } else {
        iterations = cphWorkerThreadGetIterations(pWorkerThread);
        if ( (CPHTRUE == pResponder->pConnection->isTransacted) && ((iterations+1)%pResponder->pConnection->commitCount==0) ) {
            CPHTRACEMSG(pResponder->pTrc, CPH_ROUTINE, "Calling MQCMIT");
            /*printf("cphGetOntIteration: RESPONDER: Calling MQCMIT\n");*/
            MQCMIT(pResponder->Hcon, &CompCode, &Reason);
        }

        /* transfer the ReplyToQ for the put leg */
        strncpy(pResponder->o_queue, md.ReplyToQ, MQ_Q_NAME_LENGTH); /* TODO: use default if not set?? */
        /*strncpy(pResponder->o_queue, md.ReplyToQ, sizeof(md.ReplyToQ)); *//* TODO: use default if not set?? */
        /*printf("cpgGetOneIteration: ReplyToQ=%s,o_queue=%s\n", md.ReplyToQ, pResponder->o_queue); */

        /* transfer the correlid for the put leg */
        memcpy(pResponder->put.md.CorrelId, md.CorrelId, sizeof(md.CorrelId));

    }

    CPHTRACEEXIT(pResponder->pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE
    return(status);
}

static int cphOneInteration(CPH_Responder *pResponder) {
    int status = cphGetOneInteration(pResponder);

    /*printf("cphOneIteration: RESPONDER: %d, %s\n", status, pResponder->o_queue);*/

    if (status == CPHTRUE)
    {
        status = cphPutOneInteration(pResponder);
    }
    return status;
}



