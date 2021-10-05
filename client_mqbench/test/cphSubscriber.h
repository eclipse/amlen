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
*           07-01-11   Oliver Fenton Add read ahead support
*/

#include <cmqc.h>                  /* For regular MQI definitions   */
#include <cmqxc.h>                 /* For MQCD definition           */


#include "cphWorkerThread.h"
#include "cphConnection.h"

#ifndef _CPHSUBSCRIBER

/* This is the prefix for the thread names used in logging and tracing */
#define CPH_SUBSCRIBER_THREADNAME_PREFIX "Subscriber"

/* The initial amount of memory we allocate for the message buffer. This is extended
   if we receive an MQRC_TRUNCATED_MSG failed error */
#define CPH_SUBSCRIBER_INITIALMSGBBUFFERSIZE 1024

/* The subscriber control structure */
typedef struct _cphsubscriber {
   CPH_WORKERTHREAD *pWorkerThread;                 /* Pointer to the worker thread control structure associated with this thread */
   CPH_CONNECTION *pConnection;                     /* Pointer to the cph connection structure */

   MQHCONN  Hcon;                                   /* connection handle             */
   MQHOBJ   Hobj;                                   /* object handle                 */
   MQHOBJ   Hsub;                                   /* object handle                 */
   MQHMSG   Hmsg;                                   /* message handle                */

   MQLONG   O_options;                              /* MQOPEN options                */
   MQLONG   C_options;                              /* MQCLOSE options               */
   MQLONG   messlen;                                /* message length                */
   MQBYTE   *buffer;                                /* message buffer                */
   MQLONG   bufflen;                                /* the size of the pre-allocated buffer */
   int      whetherDurable;                         /* Specifies whether we are to use durable subscriptions */
   int      useRFH2;                                /* Get forcing RFH2              */
   int      useMessageHandle;                       /* Get using message handle      */
   char     subscriberName[MQ_SUB_IDENTITY_LENGTH]; /* Subscription Id */
   int      unSubscribe;                            /* Flag which specifies whether durable subs are left intact on exit */
   int      timeout;                                /* Receive timeout in seconds    */
   CPH_TRACE *pTrc;                                 /* Pointer to the trace structure (keeping it here saves having to dereference it on every iteration */
   MQGMO    gmo;                                    /* Get message options */
   char     messageString[128];                     /* Buffer for console messages   */
   int      isReadAhead;                            /* Read Ahead flag (v7+)         */
} CPH_SUBSCRIBER;

/* Function prototypes */
void cphSubscriberIni(CPH_SUBSCRIBER **ppSubscriber, CPH_WORKERTHREAD *pWorkerThread);
int cphSubscriberFree(CPH_SUBSCRIBER **ppSubscriber);
CPH_RUN cphSubscriberRun( void * lpParam ); 
int cphSubscriberOneIteration(void *aProvider);
int cphSubscriberMakeDurableSubscriptionName(void *aProvider, char *subName);

#define _CPHSUBSCRIBER
#endif
