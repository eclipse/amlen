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

#include <cmqc.h>

#include "cphWorkerThread.h"
#include "cphConnection.h"

#ifndef _CPHPUBLISHER

/* This is the prefix for the thread names used in logging and tracing */
#define CPH_PUBLISHER_THREADNAME_PREFIX "Publisher"

/* The publisher control structure */
typedef struct _cphpublisher {
   CPH_WORKERTHREAD *pWorkerThread; /* Pointer to the worker thread control structure associated with this thread */
   CPH_CONNECTION *pConnection;     /* Pointer to the cph connection structure */


   MQHCONN  Hcon;                         /* connection handle             */
   MQHOBJ   Hobj;                         /* object handle                 */
   MQHMSG   Hmsg;                         /* message handle                */
   MQLONG   O_options;                    /* MQOPEN options                */
   MQLONG   C_options;                    /* MQCLOSE options               */
   MQLONG   messLen;                      /* message length                */
   char     *buffer;                      /* message buffer                */
   int      isPersistent;                 /* Persistency flag              */
   int      useRFH2;                      /* Publish with RFH2             */
   int      useMessageHandle;             /* Publish with message handle   */
   CPH_TRACE *pTrc;                       /* Pointer to the trace structure (keeping it here saves having to dereference it on every iteration */
   MQMD     md;                           /* Message descriptor */
   MQPMO    pmo;                          /* put message options           */  
   char     messageString[128];           /* Buffer for console messages   */
   int      oneTopicPerPublisher;         /* Publish on a different topic for each iteration */

} CPH_PUBLISHER;

/* Function prototypes */
void cphPublisherIni(CPH_PUBLISHER **ppPublisher, CPH_WORKERTHREAD *pWorkerThread);
int cphPublisherFree(CPH_PUBLISHER **ppPublisher);

CPH_RUN cphPublisherRun( void * lpParam );

int cphPublisherOneIteration(CPH_PUBLISHER *pPublisher);

#define _CPHPUBLISHER
#endif
