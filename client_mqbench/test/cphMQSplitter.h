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

#ifndef _CPHMQSPLITTER

#include "cphLog.h"

#if defined(WIN32)

#define ITS_WINDOWS

  #include <windows.h>
  #define EPTYPE HMODULE
  #ifndef MQEA
    typedef int (MQENTRY *MQFUNCPTR)();
  #else 
    #define FUNCPTR FARPROC
    typedef int (MQENTRY *MQFUNCPTR)();
  #endif
   /* EstablishMQEpsX will load either the local or remote dll according to its input parameter is Client */
   #define MQLOCAL_NAME  "mqm.dll"
   #define MQREMOTE_NAME "mqic32.dll"

/*********************************************************************/
/*   Typedefs                                                        */
/*********************************************************************/

typedef struct mq_epList
{
 MQFUNCPTR mqconn;
 MQFUNCPTR mqconnx;
 MQFUNCPTR mqdisc;
 MQFUNCPTR mqopen;
 MQFUNCPTR mqclose;
 MQFUNCPTR mqget;
 MQFUNCPTR mqput;
 MQFUNCPTR mqset;
 MQFUNCPTR mqput1;
 MQFUNCPTR mqback;
 MQFUNCPTR mqcmit;
 MQFUNCPTR mqinq;
#ifndef CPH_WMQV6
 MQFUNCPTR mqsub;
 MQFUNCPTR mqbufmh;
 MQFUNCPTR mqcrtmh;
 MQFUNCPTR mqdltmh;
#endif
} mq_epList;
 
typedef mq_epList * Pmq_epList;

#elif defined(AMQ_UNIX)

#if defined(_AIX)
#define MQLOCAL_NAME "libmqm_r.a(libmqm_r.o)"
#define MQREMOTE_NAME "libmqic_r.a(mqic_r.o)"
#elif defined(_SOLARIS_2)
#define MQLOCAL_NAME "libmqm.so"
#define MQREMOTE_NAME "libmqic.so"
#elif defined(_HPUX)
#define MQLOCAL_NAME "libmqm_r.sl"
#define MQREMOTE_NAME "libmqic_r.sl"
#else
#define MQLOCAL_NAME "libmqm_r.so"
#define MQREMOTE_NAME "libmqic_r.so"
#endif


#define EPTYPE void * 
typedef int (MQENTRY *MQFUNCPTR)();

typedef struct mq_epList
{
 MQFUNCPTR mqconn;
 MQFUNCPTR mqconnx;
 MQFUNCPTR mqdisc;
 MQFUNCPTR mqopen;
 MQFUNCPTR mqclose;
 MQFUNCPTR mqget;
 MQFUNCPTR mqput;
 MQFUNCPTR mqset;
 MQFUNCPTR mqput1;
 MQFUNCPTR mqback;
 MQFUNCPTR mqcmit;
 MQFUNCPTR mqinq;
#ifndef CPH_WMQV6
 MQFUNCPTR mqsub;
 MQFUNCPTR mqbufmh;
 MQFUNCPTR mqcrtmh;
 MQFUNCPTR mqdltmh;
#endif
} mq_epList;
 
typedef mq_epList * Pmq_epList;


#endif

/* Function prototypes */
int cphMQSplitterCheckMQLoaded(CPH_LOG *pLog, int isClient);
long cphMQSplitterLoadMQ(CPH_LOG *pLog, Pmq_epList ep, int isClient);

#define _CPHMQSPLITTER
#endif /* of _CPHMQSPLITTER */

 
