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
*          30-10-09    Leigh Taylor  File Creation
*/

/* This module is the implementation of the "Sender" test suite. This provides a set of functions that are used to put messages under
** the control of the worker thread function, which controls the rate and duration for putting messages. 
*/

#include <string.h>
#include "cphSender.h"

static CPH_RUN cphSenderRun( void * lpParam );
static int cphSenderOneIteration(CPH_Sender *pSender);



/*
** Method: cphSenderIni
**
** Create and initialise the control block for a Sender. The worker thread control structure
** is passed in as in input parameter and is stored within the Sender - which is as close as we
** can get to creating a class derived from the worker thread.
**
** Input Parameters: pWorkerThread - the worker thread control structure
**
** Output parameters: ppSender - a double pointer to the newly created Sender control structure
**
*/
void cphSenderIni(CPH_Sender **ppSender, CPH_WORKERTHREAD *pWorkerThread) {
	CPH_Sender *pSender = NULL;
    CPH_CONFIG *pConfig;
    int messLen;
    int isPersistent;
    int useRFH2;
    size_t rfh2Len = 0;
    int useMessageHandle;
    char *buffer;
    CPH_CONNECTION *pConnection;
    MQMD     md = {MQMD_DEFAULT};    /* Message Descriptor            */
    MQPMO   pmo = {MQPMO_DEFAULT};   /* put message options           */  

    int status = CPHTRUE;

    CPH_TRACE *pTrc = cphWorkerThreadGetTrace(pWorkerThread);
    pConfig = pWorkerThread->pConfig;
	
    #define CPH_ROUTINE "cphSenderIni"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

	if (NULL != pWorkerThread) {

       cphConnectionIni(&pConnection, pConfig);   

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
               if ( NULL == (buffer = cphUtilMakeBigStringWithRFH2(messLen, &rfh2Len)) ) {
                   cphConfigInvalidParameter(pConfig, "Cannot create message");
                   status = CPHFALSE;
               }
           }
           else if ( NULL == (buffer = cphUtilMakeBigString(messLen)) ) {
               cphConfigInvalidParameter(pConfig, "Cannot create message");
               status = CPHFALSE;
           }
       }

       if ( (CPHTRUE == status) && (CPHTRUE != cphConnectionParseArgs(pConnection, pWorkerThread->pControlThread->pDestinationFactory))) {
		    cphConfigInvalidParameter(pConfig, "Cannot parse options for MQ connection");
            status = CPHFALSE;

        }

       if ( (CPHTRUE == status) &&
            (NULL == (pSender = malloc(sizeof(CPH_Sender)))) ) {
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

       /******************************************************************/
       /* Ensure a new MsgId and CorrelId are created for each message.  */
       /******************************************************************/

       pmo.Options |= MQPMO_NEW_MSG_ID;
       pmo.Options |= MQPMO_NEW_CORREL_ID;

       if (CPHTRUE == pConnection->isTransacted) pmo.Options |= MQPMO_SYNCPOINT;
       
       if (CPHTRUE == status) {
          pSender->pConnection = pConnection;      
	      pSender->pWorkerThread = pWorkerThread;
          pSender->pWorkerThread->pRunFn = cphSenderRun;
	      pSender->pWorkerThread->pOneIterationFn = (int (*) (void *) ) cphSenderOneIteration;
          pSender->buffer = buffer;
          pSender->messLen = messLen;
          pSender->isPersistent = isPersistent;
          pSender->useRFH2 = useRFH2;
          if (useRFH2)
          {
            pSender->messLen += (MQLONG)rfh2Len;
          }
          pSender->useMessageHandle = useMessageHandle;
          pSender->Hmsg = MQHM_NONE;
          pSender->pTrc = pTrc;
          memcpy(&(pSender->md), &md, sizeof(MQMD));
          pmo.Version = MQPMO_VERSION_3;
          memcpy(&(pSender->pmo), &pmo, sizeof(MQPMO));

       /* Set the thread name according to our prefix */
       cphWorkerThreadSetThreadName(pWorkerThread, CPH_Sender_THREADNAME_PREFIX);
	   *ppSender = pSender;

       }
    }

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE
}

/*
** Method: cphSenderFree
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
int cphSenderFree(CPH_Sender **ppSender) {
    int res = CPHFALSE;
    CPH_TRACE *pTrc;

    /* Take a local copy of pTrc so we can call CPHTRACEEXIT after we have freed the Sender */
    pTrc = (*ppSender)->pTrc;
	
    #define CPH_ROUTINE "cphSenderFree"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

	if (NULL != *ppSender) {
        if (NULL != (*ppSender)->pConnection) cphConnectionFree(&(*ppSender)->pConnection);
        if (NULL != (*ppSender)->buffer) {
            free((*ppSender)->buffer); 
            (*ppSender)->buffer = NULL;
        }
        if (NULL != (*ppSender)->pWorkerThread) cphWorkerThreadFree( &(*ppSender)->pWorkerThread );
        free(*ppSender);
		*ppSender = NULL;
		res = CPHTRUE;
	}
    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE
	return(res);
}

/*
** Method: cphSenderRun
**
** This is the main execution method for the Sender thread. It makes the connection to MQ, 
** opens the destination and then calls the worker thread pace method. The latter is responssible for 
** calling the cphSenderOneIteration method at the correct rate according to the command line
** parameters. Once the pace method exits, cphSenderRun closes the connection to MQ and backs 
** out any messages put (if running transactionally) that were not already committed.
**
** Input Parameters: lpParam - a void * pointer to the Sender control structure. This has to be passed in as 
** a void pointer as a requirement of the thread creation routines.
**
** Returns: On windows - CPHTRUE on successful execution, CPHFALSE otherwise
**          On Linux   - NULL
**
*/
static CPH_RUN cphSenderRun( void * lpParam ) {

	CPH_Sender *pSender;
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

	pSender = lpParam;
	pWorker = pSender->pWorkerThread;
    pLog = pWorker->pConfig->pLog;

    #define CPH_ROUTINE "cphSenderRun"
    CPHTRACEENTRY(pSender->pTrc, CPH_ROUTINE );

	sprintf(pSender->messageString, "%s%s", cphWorkerThreadGetName(pWorker), " : START");
    cphLogPrintLn(pLog, LOGINFO, pSender->messageString);

    cphConnectionSetConnectOpts(pSender->pConnection, &ClientConn, &ConnectOpts);

	sprintf(pSender->messageString, "Connect to QM %s", pSender->pConnection->QMName);
    cphLogPrintLn(pLog, LOGINFO, pSender->messageString);

    MQCONNX(pSender->pConnection->QMName,     /* queue manager                  */
          &ConnectOpts,
          &(pSender->Hcon),     /* connection handle              */
          &CompCode,               /* completion code                */
          &CReason);               /* reason code                    */


    /* report reason and stop if it failed     */
    if (CompCode == MQCC_FAILED) {
      sprintf(pSender->messageString, "MQCONNX ended with reason code %d", CReason);
      cphLogPrintLn(pLog, LOGERROR, pSender->messageString );
      exit( (int)CReason );
    }

   /******************************************************************/
   /*                                                                */
   /*   Open the target queue for output                             */
   /*                                                                */
   /******************************************************************/
   
       //strncpy(od.ObjectName, pSender->topic, MQ_TOPIC_NAME_LENGTH);

   strncpy(od.ObjectName, pSender->pConnection->destination.queue, 48);
   strncpy(od.ObjectQMgrName, pSender->pConnection->QMName, 48);

   od.ObjectType = MQOT_Q;
   od.Version = MQOD_VERSION_4;

   O_options =   MQOO_OUTPUT            /* open queue for output     */
               | MQOO_FAIL_IF_QUIESCING /* but not if MQM stopping   */
               ;                        /* = 0x2010 = 8208 decimal   */

	sprintf(pSender->messageString, "Open queue %s", pSender->pConnection->destination.queue);
    cphLogPrintLn(pLog, LOGINFO, pSender->messageString);

   MQOPEN(pSender->Hcon,                      /* connection handle            */
          &od,                       /* object descriptor for queue  */
          O_options,                 /* open options                 */
          &pSender->Hobj,                     /* object handle                */
          &OpenCode,                 /* MQOPEN completion code       */
          &Reason);                  /* reason code                  */

   /* report reason, if any; stop if failed      */
   if (Reason != MQRC_NONE)
   {
      sprintf(pSender->messageString, "MQOPEN ended with reason code %d", Reason);
      cphLogPrintLn(pLog, LOGERROR, pSender->messageString );
   }

   if (OpenCode == MQCC_FAILED)
   {
      cphLogPrintLn(pLog, LOGERROR, "unable to open queue for output" );
      exit( (int)Reason );
   }

    cphWorkerThreadSetState(pWorker, sRUNNING);
	
    sprintf(pSender->messageString, "%s%s", cphWorkerThreadGetName(pWorker), " : Entering client loop");
    cphLogPrintLn(pWorker->pConfig->pLog, LOGVERBOSE, pSender->messageString);

    cphWorkerThreadPace(pWorker, pSender);
 
    iterations = cphWorkerThreadGetIterations(pWorker);
    if ( (CPHTRUE == pSender->pConnection->isTransacted) && (iterations != 0) && (iterations%pSender->pConnection->commitCount!=0) ) {
        CPHTRACEMSG(pSender->pTrc, CPH_ROUTINE, "Calling MQBACK");
        MQBACK(pSender->Hcon, &CompCode, &Reason);
    }

    if (pSender->Hmsg != MQHM_NONE)
    {
      MQDLTMH(pSender->Hcon, &pSender->Hmsg, &DltMH_options, &CompCode, &Reason);
    }

   /******************************************************************/
   /*                                                                */
   /*   Close the target queue (if it was opened)                    */
   /*                                                                */
   /******************************************************************/
   if (OpenCode != MQCC_FAILED)
   {
       C_options = MQCO_NONE;        /* no close options             */

     MQCLOSE(pSender->Hcon,                   /* connection handle            */
             &pSender->Hobj,                  /* object handle                */
             C_options,
             &CompCode,              /* completion code              */
             &Reason);               /* reason code                  */

     /* report reason, if any     */
     if (Reason != MQRC_NONE)
     {
      sprintf(pSender->messageString, "MQCLOSE ended with reason code %d", Reason);
      cphLogPrintLn(pLog, LOGERROR, pSender->messageString );
     }
   }


    /* Disconnect from the queue manager */
    if (CReason != MQRC_ALREADY_CONNECTED)  {
       MQDISC(&pSender->Hcon,                   /* connection handle            */
              &CompCode,               /* completion code              */
              &Reason);                /* reason code                  */

       /* report reason, if any     */
       if (Reason != MQRC_NONE) {
          sprintf(pSender->messageString, "MQDISC ended with reason code %d", Reason);
          cphLogPrintLn(pLog, LOGERROR, pSender->messageString );
       }
    }

	sprintf(pSender->messageString, "%s%s", cphWorkerThreadGetName(pWorker), " : STOP");
    cphLogPrintLn(pWorker->pConfig->pLog, LOGINFO, pSender->messageString);

    //state = (state&sERROR)|sENDED;
    cphWorkerThreadSetState(pWorker, (cphWorkerThreadGetState(pWorker) & sERROR) | sENDED);

    CPHTRACEEXIT(pSender->pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE

    /* Return the right type of variable according to the o/s. (This return is not used) */
    return CPH_RUNRETURN;
}

/*
** Method: cphSenderOneIteration
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
static int cphSenderOneIteration(CPH_Sender *pSender) {

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
    #define CPH_ROUTINE "cphSenderOneIteration"
    CPHTRACEENTRY(pSender->pTrc, CPH_ROUTINE );

    CPHTRACEMSG(pSender->pTrc, CPH_ROUTINE, "Message to be sent: %s", pSender->buffer);

    if ((pSender->useMessageHandle) && (pSender->Hmsg == MQHM_NONE))
    {
      MQCRTMH(pSender->Hcon,
              &CrtMHOpts,
              &pSender->Hmsg,
              &CompCode,
              &Reason);
      if (Reason != MQRC_NONE)
      {
        sprintf(pSender->messageString, "MQCRTMH ended with reason code %d", Reason);
        cphLogPrintLn(pSender->pWorkerThread->pConfig->pLog, LOGERROR, pSender->messageString );
        status = CPHFALSE;
      }

      if (status == CPHTRUE)
      {
        Rfh2Buf = cphBuildRFH2(&Rfh2Len);
        memcpy(pSender->md.Format, MQFMT_RF_HEADER_2, (size_t)MQ_FORMAT_LENGTH);

        MQBUFMH(pSender->Hcon, pSender->Hmsg, &BufMHOpts, &pSender->md, Rfh2Len, Rfh2Buf, &DataLen, &CompCode, &Reason);

        /* report reason, if any */
        if (Reason != MQRC_NONE) {
             sprintf(pSender->messageString, "MQBUFMH ended with reason code %d\n", Reason);
             cphLogPrintLn(pSender->pWorkerThread->pConfig->pLog, LOGERROR, pSender->messageString );
             status = CPHFALSE;
        }

        free(Rfh2Buf);
      }
    }
	else
	{
       buffer2 = malloc(pSender->messLen);
	   memcpy(buffer2, pSender->buffer, pSender->messLen);
	   userfh2 = CPHTRUE;
	}

    if (status == CPHTRUE)
    {

      pSender->pmo.OriginalMsgHandle = pSender->Hmsg;
    MQPUT(pSender->Hcon,       /* connection handle               */
             pSender->Hobj,    /* object handle                   */
             &pSender->md,     /* message descriptor              */
             &pSender->pmo,    /* default options (datagram)      */
             pSender->messLen, /* message length                  */
			 (userfh2 ? buffer2 : pSender->buffer),  /* message buffer                  */
             &CompCode,           /* completion code                 */
             &Reason);            /* reason code                     */
    /* report reason, if any */
    if (Reason != MQRC_NONE) {
         sprintf(pSender->messageString, "MQPUT ended with reason code %d\n", Reason);
         cphLogPrintLn(pSender->pWorkerThread->pConfig->pLog, LOGERROR, pSender->messageString );
         status = CPHFALSE;
    } else {
        if ( (CPHTRUE == pSender->pConnection->isTransacted) && ((cphWorkerThreadGetIterations(pSender->pWorkerThread)+1)%pSender->pConnection->commitCount==0) ) {
            CPHTRACEMSG(pSender->pTrc, CPH_ROUTINE, "Calling MQCMIT");
            MQCMIT(pSender->Hcon, &CompCode, &Reason);
        }
    }
    }

	free(buffer2);


	CPHTRACEEXIT(pSender->pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE
    return(status);
}
