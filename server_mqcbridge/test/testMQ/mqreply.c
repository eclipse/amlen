/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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

/**********************************************************************/
/* MQTT client request / response - MQ responder                      */
/*                                                                    */
/* Usage: Usage: mqreply <topic> <qmgr>                               */
/*                                                                    */
/* Subscribes to <topic> on <qmgr>                                    */
/* Puts responses to messages to ReplyToQMgr                          */
/* Intended to be used with ...                                       */
/**********************************************************************/

/**********************************************************************/
/* Compile Windows:                                                   */
/*   CL /Zi mqresponse.c mqm.lib                                      */
/* Compile Linux:                                                     */
/*   gcc -o mqresponse mqresponse.c -lmqm_r -I/opt/mqm/inc            */
/**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmqc.h>

MQLONG consumer(MQHCONN hConn,
                PMQMD   pMd,
                PMQGMO  pGmo,
                PMQVOID pBuffer,
                PMQCBC  pContext)
{
  char * pMessage;
  int messagelen;
  MQOD od = {MQOD_DEFAULT};
  MQMD md = {MQMD_DEFAULT};
  MQPMO pmo = {MQPMO_DEFAULT};
  MQLONG CompCode, Reason;

  if (pContext -> CallType == MQCBCT_MSG_REMOVED)
  {
    printf("pMd -> ReplyToQMgr '%.48s'\n", pMd -> ReplyToQMgr);

    strcpy(od.ObjectName, "OBJECTNAME");  
    memcpy(od.ObjectQMgrName, pMd -> ReplyToQMgr, MQ_Q_NAME_LENGTH);

    messagelen = strlen("REPLY: ") + pGmo -> ReturnedLength;
    pMessage = malloc(messagelen);
    memcpy(pMessage, "REPLY: ", strlen("REPLY: "));
    memcpy(pMessage + strlen("REPLY: "), (char *)pBuffer, pGmo -> ReturnedLength);

    MQPUT1(hConn,
           &od,
           &md,
           &pmo,
           messagelen,
           pMessage,
           &CompCode,
           &Reason);
    printf("MQPUT1 CompCode(%d) Reason(%d)\n", CompCode, Reason);
  }

  return 0;
}

int main(int argc, char **argv)
{
  MQSD    sd = {MQSD_DEFAULT};
  MQMD    md = {MQMD_DEFAULT};
  MQGMO   gmo = {MQGMO_DEFAULT};
  MQCBD   cbd = {MQCBD_DEFAULT};
  MQCTLO  ctlo = {MQCTLO_DEFAULT};
  MQHCONN hConn;
  MQHOBJ  hObj, hSub;
  MQLONG  CompCode, Reason;
  int     rc = 0, ch;

  if (argc < 3)
  {
    printf("Usage: mqresponse <topic> <qmgr>\n");
    rc = 1;
    goto MOD_EXIT;
  }

  /* MQCONN */
  MQCONN(argv[2],
         &hConn,
         &CompCode,
         &Reason);
  printf("MQCONN CompCode(%d) Reason(%d)\n", CompCode, Reason);

  /* MQSUB */
  sd.Options = MQSO_CREATE | MQSO_NON_DURABLE | MQSO_MANAGED;

  sd.ObjectString.VSPtr = argv[1];
  sd.ObjectString.VSLength = strlen(argv[1]);

  MQSUB(hConn,
        &sd,
        &hObj,
        &hSub,
        &CompCode,
        &Reason);
  printf("MQSUB CompCode(%d) Reason(%d)\n", CompCode, Reason);

  /* MQCB */
  cbd.CallbackFunction = consumer;

  MQCB(hConn,
       MQOP_REGISTER,
       &cbd,
       hObj,
       &md,
       &gmo,
       &CompCode,
       &Reason);
  printf("MQCB CompCode(%d) Reason(%d)\n", CompCode, Reason);

  /* MQCTL(START) */
  MQCTL(hConn,
        MQOP_START,
        &ctlo,
        &CompCode,
        &Reason);
  printf("MQCTL(START) Compcode(%d) Reason(%d)\n", CompCode, Reason);

  /* wait for input */
  do
  {
    ch = getchar();
  } while(ch!='Q' && ch != 'q');

  /* MQCTL(STOP) */
  MQCTL(hConn,
        MQOP_STOP,
        &ctlo,
        &CompCode,
        &Reason);
  printf("MQCTL(STOP) Compcode(%d) Reason(%d)\n", CompCode, Reason);

  /* MQCLOSE(hSub) */
  MQCLOSE(hConn,
          &hSub,
          MQCO_NONE,
          &CompCode,
          &Reason);
  printf("MQCLOSE CompCode(%d) Reason(%d)\n", CompCode, Reason);

  /* MQCLOSE(hObj) */
  MQCLOSE(hConn,
          &hObj,
          MQCO_NONE,
          &CompCode,
          &Reason);
  printf("MQCLOSE CompCode(%d) Reason(%d)\n", CompCode, Reason);

  /* MQDISC */
  MQDISC(&hConn,
         &CompCode,
         &Reason);
  printf("MQDISC CompCode(%d) Reason(%d)\n", CompCode, Reason);

MOD_EXIT:
  return rc;
}

