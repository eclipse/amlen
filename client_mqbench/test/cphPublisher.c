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
*          12-04-10    Rachel Norris Added support for multiple topics per Publisher
*/

/* This module is the implementation of the "Publisher" test suite. This provides a set of functions that are used to publish messages under
** the control of the worker thread function, which controls the rate and duration for publish messages. 
*/

#include <string.h>
#include "cphPublisher.h"

/*
** Method: cphPublisherIni
**
** Create and initialise the control block for a Publisher. The worker thread control structure
** is passed in as in input parameter and is stored within the publisher - which is as close as we
** can get to creating a class derived from the worker thread.
**
** Input Parameters: pWorkerThread - the worker thread control structure
**
** Output parameters: ppPublisher - a double pointer to the newly created publisher control structure
**
*/
void cphPublisherIni(CPH_PUBLISHER **ppPublisher, CPH_WORKERTHREAD *pWorkerThread) {
	CPH_PUBLISHER *pPublisher = NULL;
    CPH_CONFIG *pConfig;
    int messLen;
    int isPersistent;
    int useRFH2;
    size_t rfh2Len = 0;
    int useMessageHandle;
	int oneTopicPerPublisher;
    char *buffer;
    CPH_CONNECTION *pConnection;
    MQMD     md = {MQMD_DEFAULT};    /* Message Descriptor            */
    MQPMO   pmo = {MQPMO_DEFAULT};   /* put message options           */  

    int status = CPHTRUE;

    CPH_TRACE *pTrc = cphWorkerThreadGetTrace(pWorkerThread);
    pConfig = pWorkerThread->pConfig;
	
    #define CPH_ROUTINE "cphPublisherIni"
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

       if ( (CPHTRUE == status) && (CPHTRUE != cphConfigGetBoolean(pConfig, &oneTopicPerPublisher, "tp"))) {
          cphConfigInvalidParameter(pConfig, "Cannot retrieve topics per publisher option");
            status = CPHFALSE;
       }
       else {
           CPHTRACEMSG(pTrc, CPH_ROUTINE, "oneTopicPerPublisher: %d.", oneTopicPerPublisher);

       }

       if ( (CPHTRUE == status) &&
            (NULL == (pPublisher = malloc(sizeof(CPH_PUBLISHER)))) ) {
		    cphConfigInvalidParameter(pConfig, "Cannot allocate memory for publisher control block");
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
       /* The MsgId and CorrelId of the publication messages sent to the */
       /* subscribers are always replaced by the Publish/Subscribe       */
       /* engine at publication time.                                    */
       /******************************************************************/
       pmo.Options |= MQPMO_NEW_MSG_ID;
       pmo.Options |= MQPMO_NEW_CORREL_ID;

       if (CPHTRUE == pConnection->isTransacted) pmo.Options |= MQPMO_SYNCPOINT;
       
       if (CPHTRUE == status) {
          pPublisher->pConnection = pConnection;      
	      pPublisher->pWorkerThread = pWorkerThread;
          pPublisher->pWorkerThread->pRunFn = cphPublisherRun;
	      pPublisher->pWorkerThread->pOneIterationFn = (int (*) (void *) ) cphPublisherOneIteration;
          pPublisher->buffer = buffer;
          pPublisher->messLen = messLen;
          pPublisher->isPersistent = isPersistent;
          pPublisher->useRFH2 = useRFH2;
          if (useRFH2)
          {
            pPublisher->messLen += (MQLONG)rfh2Len;
          }
          pPublisher->useMessageHandle = useMessageHandle;
		  pPublisher->oneTopicPerPublisher = oneTopicPerPublisher;
          pPublisher->Hmsg = MQHM_NONE;
          pPublisher->pTrc = pTrc;
          memcpy(&(pPublisher->md), &md, sizeof(MQMD));
          pmo.Version = MQPMO_VERSION_3;
          memcpy(&(pPublisher->pmo), &pmo, sizeof(MQPMO));

       /* Set the thread name according to our prefix */
       cphWorkerThreadSetThreadName(pWorkerThread, CPH_PUBLISHER_THREADNAME_PREFIX);
	   *ppPublisher = pPublisher;

       }
    }

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE
}

/*
** Method: cphPublisherFree
**
** Free the publisher control block. The routine that disposes of the control thread connection block calls 
** this function for every publisher worker thread.
**
** Input/output Parameters: Double pointer to the publisher. When the publisher control
** structure is freed, the single pointer to the control structure is returned as NULL.
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphPublisherFree(CPH_PUBLISHER **ppPublisher) {
    int res = CPHFALSE;
    CPH_TRACE *pTrc;

    /* Take a local copy of pTrc so we can call CPHTRACEEXIT after we have freed the publisher */
    pTrc = (*ppPublisher)->pTrc;
	
    #define CPH_ROUTINE "cphPublisherFree"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

	if (NULL != *ppPublisher) {
        if (NULL != (*ppPublisher)->pConnection) cphConnectionFree(&(*ppPublisher)->pConnection);
        if (NULL != (*ppPublisher)->buffer) {
            free((*ppPublisher)->buffer); 
            (*ppPublisher)->buffer = NULL;
        }
        if (NULL != (*ppPublisher)->pWorkerThread) cphWorkerThreadFree( &(*ppPublisher)->pWorkerThread );
        free(*ppPublisher);
		*ppPublisher = NULL;
		res = CPHTRUE;
	}
    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE
	return(res);
}

/*
** Method: cphPublisherRun
**
** This is the main execution method for the publisher thread. It makes the connection to MQ, 
** opens the destination and then calls the worker thread pace method. The latter is responssible for 
** calling the cphPublisherOneIteration method at the correct rate according to the command line
** parameters. Once the pace method exits, cphPublisherRun closes the connection to MQ and backs 
** out any messages published (if running transactionally) that were not already committed.
**
** Input Parameters: lpParam - a void * pointer to the publisher control structure. This has to be passed in as 
** a void pointer as a requirement of the thread creation routines.
**
** Returns: On windows - CPHTRUE on successful execution, CPHFALSE otherwise
**          On Linux   - NULL
**
*/
CPH_RUN cphPublisherRun( void * lpParam ) {

	CPH_PUBLISHER *pPublisher;
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

	pPublisher = lpParam;
	pWorker = pPublisher->pWorkerThread;
    pLog = pWorker->pConfig->pLog;

    #define CPH_ROUTINE "cphPublisherRun"
    CPHTRACEENTRY(pPublisher->pTrc, CPH_ROUTINE );

	sprintf(pPublisher->messageString, "%s%s", cphWorkerThreadGetName(pWorker), " : START");
    cphLogPrintLn(pLog, LOGINFO, pPublisher->messageString);

    cphConnectionSetConnectOpts(pPublisher->pConnection, &ClientConn, &ConnectOpts);

    MQCONNX(pPublisher->pConnection->QMName,     /* queue manager                  */
          &ConnectOpts,
          &(pPublisher->Hcon),     /* connection handle              */
          &CompCode,               /* completion code                */
          &CReason);               /* reason code                    */


    /* report reason and stop if it failed     */
    if (CompCode == MQCC_FAILED) {
      sprintf(pPublisher->messageString, "MQCONNX ended with reason code %d", CReason);
      cphLogPrintLn(pLog, LOGERROR, pPublisher->messageString );
      exit( (int)CReason );
    }

   /******************************************************************/
   /*                                                                */
   /*   Open the target topic for output                             */
   /*                                                                */
   /******************************************************************/

    // Only do the Open here if we are running in one topic per publisher mode ( default )
    if ( pPublisher->oneTopicPerPublisher == CPHTRUE )
    {
       od.ObjectString.VSPtr=pPublisher->pConnection->destination.topic;
       od.ObjectString.VSLength=(MQLONG)strlen(pPublisher->pConnection->destination.topic);   
       od.ObjectType = MQOT_TOPIC;
       od.Version = MQOD_VERSION_4;

       O_options =   MQOO_OUTPUT            /* open queue for output     */
                   | MQOO_FAIL_IF_QUIESCING /* but not if MQM stopping   */
                   ;                        /* = 0x2010 = 8208 decimal   */

       MQOPEN(pPublisher->Hcon,          /* connection handle            */
              &od,                       /* object descriptor for queue  */
              O_options,                 /* open options                 */
              &pPublisher->Hobj,         /* object handle                */
              &OpenCode,                 /* MQOPEN completion code       */
              &Reason);                  /* reason code                  */

       /* report reason, if any; stop if failed      */
       if (Reason != MQRC_NONE)
       {
           sprintf(pPublisher->messageString, "MQOPEN ended with reason code %d", Reason);
           cphLogPrintLn(pLog, LOGERROR, pPublisher->messageString );
       }

       if (OpenCode == MQCC_FAILED)
       {
           cphLogPrintLn(pLog, LOGERROR, "unable to open queue for output" );
           exit( (int)Reason );
       }
    }

    cphWorkerThreadSetState(pWorker, sRUNNING);
	
    sprintf(pPublisher->messageString, "%s%s", cphWorkerThreadGetName(pWorker), " : Entering client loop");
    cphLogPrintLn(pWorker->pConfig->pLog, LOGVERBOSE, pPublisher->messageString);

    cphWorkerThreadPace(pWorker, pPublisher);
 
    iterations = cphWorkerThreadGetIterations(pWorker);
    if ( (CPHTRUE == pPublisher->pConnection->isTransacted) && (iterations != 0) && (iterations%pPublisher->pConnection->commitCount!=0) ) {
        CPHTRACEMSG(pPublisher->pTrc, CPH_ROUTINE, "Calling MQBACK");
        MQBACK(pPublisher->Hcon, &CompCode, &Reason);
    }

    if (pPublisher->Hmsg != MQHM_NONE)
    {
      MQDLTMH(pPublisher->Hcon, &pPublisher->Hmsg, &DltMH_options, &CompCode, &Reason);
    }

   /******************************************************************/
   /*                                                                */
   /*   Close the target queue (if it was opened)                    */
   /*                                                                */
   /******************************************************************/
    if ( pPublisher->oneTopicPerPublisher == CPHTRUE )
    {
       if (OpenCode != MQCC_FAILED)
       {
           C_options = MQCO_NONE;          /* no close options             */

           MQCLOSE(pPublisher->Hcon,       /* connection handle            */
                   &pPublisher->Hobj,      /* object handle                */
                   C_options,
                   &CompCode,              /* completion code              */
                   &Reason);               /* reason code                  */

           /* report reason, if any     */
           if (Reason != MQRC_NONE)
           {
               sprintf(pPublisher->messageString, "MQCLOSE ended with reason code %d", Reason);
               cphLogPrintLn(pLog, LOGERROR, pPublisher->messageString );
           }
      }
    }

    /* Disconnect from the queue manager */
    if (CReason != MQRC_ALREADY_CONNECTED)  {
       MQDISC(&pPublisher->Hcon,                   /* connection handle            */
              &CompCode,               /* completion code              */
              &Reason);                /* reason code                  */

       /* report reason, if any     */
       if (Reason != MQRC_NONE) {
          sprintf(pPublisher->messageString, "MQDISC ended with reason code %d", Reason);
          cphLogPrintLn(pLog, LOGERROR, pPublisher->messageString );
       }
    }

	sprintf(pPublisher->messageString, "%s%s", cphWorkerThreadGetName(pWorker), " : STOP");
    cphLogPrintLn(pWorker->pConfig->pLog, LOGINFO, pPublisher->messageString);

    //state = (state&sERROR)|sENDED;
    cphWorkerThreadSetState(pWorker, (cphWorkerThreadGetState(pWorker) & sERROR) | sENDED);

    CPHTRACEEXIT(pPublisher->pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE

    /* Return the right type of variable according to the o/s. (This return is not used) */
    return CPH_RUNRETURN;
}

/*
** Method: cphPublisherOneIteration
**
** This method is called by cphWorkerThreadPace to perform a single publish operation. It is a
** critical method in terms of its implementation from a performance point of view. It performs
** a single MQ put to the target destination. (The connection to MQ has already been prepared in 
** in the cphPublisherRun method). If we are running transactionally and the number of published 
** messages has reached the commit count then the method also issues a commit call.
**
** Input Parameters: a void pointer to the publisher control structure
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphPublisherOneIteration(CPH_PUBLISHER *pPublisher) {

    MQCMHO   CrtMHOpts = {MQCMHO_DEFAULT};
    MQBMHO   BufMHOpts = {MQBMHO_DEFAULT};
    MQLONG   Rfh2Len;
    char     *Rfh2Buf;
    MQLONG   DataLen;
    MQLONG   CompCode;               /* completion code               */
    MQLONG   Reason;                 /* reason code                   */
	char * buffer2 = NULL;
	int userfh2=CPHFALSE;

    /*   Declare MQI structures needed for Open and Close             */
    MQOD     od = {MQOD_DEFAULT};    /* Object Descriptor             */
    MQLONG   O_options;              /* MQOPEN options                */ 
    MQLONG   C_options;              /* MQCLOSE options               */
    MQLONG   OpenCode;               /* MQOPEN completion code        */

    int status = CPHTRUE;
    #define CPH_ROUTINE "cphPublisherOneIteration"
    CPHTRACEENTRY(pPublisher->pTrc, CPH_ROUTINE );

   /* If we are running in the mode where each publisher publishes on multiple topics then the open has to happen on every iteration */	 
   if ( pPublisher->oneTopicPerPublisher == CPHFALSE )
   {
	  od.ObjectString.VSPtr=pPublisher->pConnection->destination.topic;
	  od.ObjectString.VSLength=(MQLONG)strlen(pPublisher->pConnection->destination.topic);   
	  od.ObjectType = MQOT_TOPIC;
      od.Version = MQOD_VERSION_4;

	  O_options =   MQOO_OUTPUT            /* open queue for output     */
                  | MQOO_FAIL_IF_QUIESCING /* but not if MQM stopping   */
                  ;                        /* = 0x2010 = 8208 decimal   */

	  MQOPEN(pPublisher->Hcon,       /* connection handle            */
          &od,                       /* object descriptor for queue  */
          O_options,                 /* open options                 */
          &pPublisher->Hobj,         /* object handle                */
          &OpenCode,                 /* MQOPEN completion code       */
          &Reason);                  /* reason code                  */

	 /* report reason, if any; stop if failed      */
	 if (Reason != MQRC_NONE)
	 {
	    sprintf(pPublisher->messageString, "MQOPEN ended with reason code %d", Reason);
		  cphLogPrintLn(pPublisher->pWorkerThread->pConfig->pLog, LOGERROR, pPublisher->messageString );
     }

	 if (OpenCode == MQCC_FAILED)
	 {
	    cphLogPrintLn(pPublisher->pWorkerThread->pConfig->pLog, LOGERROR, "unable to open queue for output" );
	    exit( (int)Reason );
	 }
   }


    CPHTRACEMSG(pPublisher->pTrc, CPH_ROUTINE, "Message to be sent: %s", pPublisher->buffer);

    if ((pPublisher->useMessageHandle) && (pPublisher->Hmsg == MQHM_NONE))
    {
      MQCRTMH(pPublisher->Hcon,
              &CrtMHOpts,
              &pPublisher->Hmsg,
              &CompCode,
              &Reason);
      if (Reason != MQRC_NONE)
      {
        sprintf(pPublisher->messageString, "MQCRTMH ended with reason code %d", Reason);
        cphLogPrintLn(pPublisher->pWorkerThread->pConfig->pLog, LOGERROR, pPublisher->messageString );
        status = CPHFALSE;
      }

      if (status == CPHTRUE)
      {
        Rfh2Buf = cphBuildRFH2(&Rfh2Len);
        memcpy(pPublisher->md.Format, MQFMT_RF_HEADER_2, (size_t)MQ_FORMAT_LENGTH);

        MQBUFMH(pPublisher->Hcon, pPublisher->Hmsg, &BufMHOpts, &pPublisher->md, Rfh2Len, Rfh2Buf, &DataLen, &CompCode, &Reason);

        /* report reason, if any */
        if (Reason != MQRC_NONE) {
             sprintf(pPublisher->messageString, "MQBUFMH ended with reason code %d\n", Reason);
             cphLogPrintLn(pPublisher->pWorkerThread->pConfig->pLog, LOGERROR, pPublisher->messageString );
             status = CPHFALSE;
        }

        free(Rfh2Buf);
      }
    }
	else
	{
       buffer2 = malloc(pPublisher->messLen);
	   memcpy(buffer2, pPublisher->buffer, pPublisher->messLen);
	   userfh2 = CPHTRUE;
	}

    if (status == CPHTRUE)
    {

      pPublisher->pmo.OriginalMsgHandle = pPublisher->Hmsg;
    MQPUT(pPublisher->Hcon,       /* connection handle               */
             pPublisher->Hobj,    /* object handle                   */
             &pPublisher->md,     /* message descriptor              */
             &pPublisher->pmo,    /* default options (datagram)      */
             pPublisher->messLen, /* message length                  */
			 (userfh2 ? buffer2 : pPublisher->buffer),  /* message buffer                  */
             &CompCode,           /* completion code                 */
             &Reason);            /* reason code                     */
    /* report reason, if any */
    if (Reason != MQRC_NONE) {
         sprintf(pPublisher->messageString, "MQPUT ended with reason code %d\n", Reason);
         cphLogPrintLn(pPublisher->pWorkerThread->pConfig->pLog, LOGERROR, pPublisher->messageString );
         status = CPHFALSE;
    } else {
        if ( (CPHTRUE == pPublisher->pConnection->isTransacted) && ((cphWorkerThreadGetIterations(pPublisher->pWorkerThread)+1)%pPublisher->pConnection->commitCount==0) ) {
            CPHTRACEMSG(pPublisher->pTrc, CPH_ROUTINE, "Calling MQCMIT");
            MQCMIT(pPublisher->Hcon, &CompCode, &Reason);
        }
    }
    }
    if (userfh2)
		free(buffer2);

   /******************************************************************/
   /*                                                                */
   /*   Close the target queue (if it was opened)                    */
   /*                                                                */
   /******************************************************************/
   if ( pPublisher->oneTopicPerPublisher == CPHFALSE )
   {
       if (OpenCode != MQCC_FAILED)
       {
           C_options = MQCO_NONE;        /* no close options             */

           MQCLOSE(pPublisher->Hcon,     /* connection handle            */    
                   &pPublisher->Hobj,    /* object handle                */
                   C_options,
                   &CompCode,           /* completion code              */
                   &Reason);             /* reason code                  */

           /* report reason, if any     */
           if (Reason != MQRC_NONE)
           {
               sprintf(pPublisher->messageString, "MQCLOSE ended with reason code %d", Reason);
               cphLogPrintLn(pPublisher->pWorkerThread->pConfig->pLog, LOGERROR, pPublisher->messageString );
           }
       }
       /******************************************************************/
       /*                                                                */
       /*   Get the next topic ready for the next iteration              */
       /*                                                                */
       /******************************************************************/
   
       if ( (CPHTRUE == status) && 
            (CPHFALSE == cphDestinationFactoryGenerateDestination(pPublisher->pWorkerThread->pControlThread->pDestinationFactory, pPublisher->pConnection->destination.topic))) {
                cphLogPrintLn(pPublisher->pWorkerThread->pConfig->pLog, LOGERROR, "Could not generate destination name" );
                status = CPHFALSE;
       }
   } 

    CPHTRACEEXIT(pPublisher->pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE
    return(status);
}
