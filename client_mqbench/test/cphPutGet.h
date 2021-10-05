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
*           5-11-09    Leigh Taylor  File Creation
*           07-01-11   Oliver Fenton Add read ahead support
*/

#include <cmqc.h>

#include "cphWorkerThread.h"
#include "cphConnection.h"

#ifndef _CPHPutGet

/* This is the prefix for the thread names used in logging and tracing */
#define CPH_PutGet_THREADNAME_PREFIX "PutGet"

/* The initial amount of memory we allocate for the message buffer. This is extended
   if we receive an MQRC_TRUNCATED_MSG failed error */
#define CPH_Receiver_INITIALMSGBBUFFERSIZE 1024

/* The PutGet control structure */
typedef struct _cphPutGet {
   CPH_WORKERTHREAD *pWorkerThread; /* Pointer to the worker thread control structure associated with this thread */
   CPH_CONNECTION *pConnection;     /* Pointer to the cph connection structure */


   MQHCONN  Hcon;                         /* connection handle             */
   MQHOBJ   Hobj;                         /* object handle                 */
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



} CPH_PutGet;

/* Function prototypes */
void cphPutGetIni(CPH_PutGet **ppPutGet, CPH_WORKERTHREAD *pWorkerThread);
int cphPutGetFree(CPH_PutGet **ppPutGet);

#define _CPHPutGet
#endif
