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
*           110513     fentono       File Creation
*/

#include <cmqc.h>

#include "cphWorkerThread.h"
#include "cphConnection.h"

#ifndef _CPHRequester

/* This is the prefix for the thread names used in logging and tracing */
#define CPH_Requester_THREADNAME_PREFIX "Requester"

/* The initial amount of memory we allocate for the message buffer. This is extended
   if we receive an MQRC_TRUNCATED_MSG failed error */
#define CPH_Receiver_INITIALMSGBBUFFERSIZE 1024

/* The Requester control structure */
typedef struct _cphRequester {
   CPH_WORKERTHREAD *pWorkerThread; /* Pointer to the worker thread control structure associated with this thread */
   CPH_CONNECTION *pConnection;     /* Pointer to the cph connection structure */

   /* for the requester & responder apps, an in-queue & an out-queue */
   /* are used so we ignore the pConnection->destination value and   */
   /* use privately defined values (-iq & -oq)                       */
   char i_queue[MQ_Q_NAME_LENGTH];
   char o_queue[MQ_Q_NAME_LENGTH];

   MQHCONN  Hcon;                         /* connection handle             */

   MQHOBJ   i_Hobj;                         /* in-queue object handle      */
   MQHOBJ   o_Hobj;                         /* out-queue object handle     */

   CPH_TRACE *pTrc;                       /* Pointer to the trace structure (keeping it here saves having to dereference it on every iteration */
   int      useRFH2;                      /* Put / Force get with RFH2     */
   int      useMessageHandle;             /* Put/get with message handle   */
   char     messageString[128];           /* Buffer for console messages   */
   MQBYTE24 CorrelId;                     /* We need this to select incoming messages */

   struct
   {
   MQHMSG   Hmsg;                         /* message handle                */
   MQLONG   messLen;                      /* message length                */
   char     *buffer;                      /* message buffer                */
   int      isPersistent;                 /* Persistency flag              */
   MQMD     md;                           /* Message descriptor */
   MQPMO    pmo;                          /* put message options           */  
   } put;

   struct
   {
   MQHMSG   Hmsg;                                   /* message handle                */

   MQLONG   messlen;                                /* message length                */
   MQBYTE   *buffer;                                /* message buffer                */
   MQLONG   bufflen;                                /* the size of the pre-allocated buffer */
   int      timeout;                                /* Receive timeout in seconds    */
   MQGMO    gmo;                                    /* Get message options */
   int      isReadAhead;                  			/* Read Ahead flag (v7+)         */
   } get;



} CPH_Requester;

/* Function prototypes */
void cphRequesterIni(CPH_Requester **ppRequester, CPH_WORKERTHREAD *pWorkerThread);
int cphRequesterFree(CPH_Requester **ppRequester);

#define _CPHRequester
#endif
