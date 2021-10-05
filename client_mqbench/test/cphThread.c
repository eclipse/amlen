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
*/

/* Thread management functions */

#include <stdio.h>

#include "cphThread.h"
#if defined(AMQ_UNIX)
#include <pthread.h>
#endif

/*
** Method: cphThreadStart
**
** This method starts a new thread using the given pointer to a function name as the thread
** entry point
**
** Input Parameters: pConfig - routine - pointer to the function to be the thread entry point
**                   arg - void pointer which is passed to the new thread as an argument.
**
** Output Parameters: pThreadId - a pointer to the thread id for the newly created threat
**
** Returns 0 on successful execution and non zero otherwise
**
**
*/
#if defined(WIN32)
int cphThreadStart(CPH_THREADID* pThreadId, CPH_STARTROUTINE routine, void *arg, CPH_THREAD_ATTR *pAttr) {
#elif defined(AMQ_UNIX)
int cphThreadStart(CPH_THREADID* pThreadId, void * (*routine) (void *), void *arg, CPH_THREAD_ATTR *pAttr) {
#endif

int rc = 0;

#if defined(WIN32)
HANDLE h;

if (NULL == ( h = CreateThread(NULL, 0, routine, arg, 0, pThreadId ))) 
   rc = -1;
#elif defined(AMQ_UNIX)
   rc = pthread_create(pThreadId, pAttr, routine, arg);

#endif

return rc;
}

/*
** Method: cphThreadYield
**
** This function is used to voluntarily yield the processor 
**
** Output parameters: ppConection - a double pointer to the newly created connection structure
**
** On Linux this returns zero on successful execution and -1 otherwise
** On Windows we always return zero
*/
int cphThreadYield(void) {

int rc = 0;

#if defined(WIN32)
YieldProcessor();
#elif defined(AMQ_UNIX)
rc = sched_yield();
#endif

return rc;
}


#ifdef AMQ_UNIX
int cphThreadIniAttr(CPH_THREAD_ATTR *pAttr)
{
int rc;

rc = pthread_attr_init(pAttr);
return rc;
}

int cphThreadSetStackSize(CPH_THREAD_ATTR *pAttr, int stackSize)
{
int rc;

rc = pthread_attr_setstacksize(pAttr, stackSize);
return rc;
}
#endif

