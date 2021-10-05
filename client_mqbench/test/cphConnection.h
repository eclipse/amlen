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
*/

#ifndef _CPHCONNECTION

#include <cmqc.h>                  /* For regular MQI definitions   */
#include <cmqxc.h>                 /* For MQCD definition           */

#include "cphDestinationFactory.h"
#include "cphConfig.h"

/* The connection control structure */
typedef struct _cphconnection {

   CPH_CONFIG *pConfig;

   char     QMName[MQ_Q_MGR_NAME_LENGTH];          /* queue manager name            */
   union {
   char     topic[MQ_TOPIC_NAME_LENGTH];           /* topic                         */
   char     queue[MQ_Q_NAME_LENGTH];               /* queue                         */
   } destination;
   int      isPersistent;                          /* Persistency flag              */
   int      isTransacted;                          /* Set if we are running in transactional mode */
   int      commitCount;                           /* The number of operations completed within a single transaction */

   int      isClientConnection;                   /* Whether we are using client or bindings connection */
   char     channelName[MQ_CHANNEL_NAME_LENGTH];  /* The default channel name */
   char     hostName[80];                         /* The hostname */
   int      portNum;                              /* The port number */
   int      isFastPath;                           /* Whether we are using the fastpath option on the MQCONNX call */

} CPH_CONNECTION;

/* Function prototypes */
void cphConnectionIni(CPH_CONNECTION **ppConnection, CPH_CONFIG *pConfig);
int cphConnectionFree(CPH_CONNECTION **ppConnection);

int cphConnectionParseArgs(CPH_CONNECTION *pConnection, CPH_DESTINATIONFACTORY *pDestination);
int cphConnectionSetConnectOpts(CPH_CONNECTION *pConnection, MQCD *pClientConn, MQCNO *pConnectOpts);

#define _CPHCONNECTION
#endif
