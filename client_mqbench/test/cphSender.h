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

#include <cmqc.h>

#include "cphWorkerThread.h"
#include "cphConnection.h"

#ifndef _CPHSender

/* This is the prefix for the thread names used in logging and tracing */
#define CPH_Sender_THREADNAME_PREFIX "Sender"

/* The Sender control structure */
typedef struct _cphSender {
   CPH_WORKERTHREAD *pWorkerThread; /* Pointer to the worker thread control structure associated with this thread */
   CPH_CONNECTION *pConnection;     /* Pointer to the cph connection structure */


   MQHCONN  Hcon;                         /* connection handle             */
   MQHOBJ   Hobj;                         /* object handle                 */
   MQHMSG   Hmsg;                         /* message handle                */
   MQLONG   messLen;                      /* message length                */
   char     *buffer;                      /* message buffer                */
   int      isPersistent;                 /* Persistency flag              */
   int      useRFH2;                      /* Publish with RFH2             */
   int      useMessageHandle;             /* Publish with message handle   */
   CPH_TRACE *pTrc;                       /* Pointer to the trace structure (keeping it here saves having to dereference it on every iteration */
   MQMD     md;                           /* Message descriptor */
   MQPMO    pmo;                          /* put message options           */  
   char     messageString[128];           /* Buffer for console messages   */

} CPH_Sender;

/* Function prototypes */
void cphSenderIni(CPH_Sender **ppSender, CPH_WORKERTHREAD *pWorkerThread);
int cphSenderFree(CPH_Sender **ppSender);

#define _CPHSender
#endif
