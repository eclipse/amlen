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

#ifndef _CPHTHREAD

#ifdef WIN32 
   #include <Windows.h>

   #define CPH_THREADID     DWORD
   #define CPH_STARTROUTINE LPTHREAD_START_ROUTINE
#elif defined(AMQ_UNIX)
   #include <pthread.h>
   #include <sched.h>
 
   #define CPH_THREADID     pthread_t

#endif

/* CPH_RUN is the return type from a function used as a thread entry point 
**
** CPH_RUNRETURN is the appropriate return type for a thread entry function.
** This return value is o/s specific - void * on Linux and DWORD WINAPI on Windows
*/
#if defined(WIN32)
#define CPH_RUN DWORD WINAPI
#define CPH_RUNRETURN CPHTRUE

#define CPH_THREAD_ATTR void
#elif defined(AMQ_UNIX)
#define CPH_RUN void *
#define CPH_RUNRETURN NULL

#define CPH_THREAD_ATTR pthread_attr_t

#endif

#if defined(WIN32)
int cphThreadStart(CPH_THREADID* threadId, CPH_STARTROUTINE routine, void *arg, CPH_THREAD_ATTR *pAttr);
#elif defined(AMQ_UNIX)
int cphThreadIniAttr(CPH_THREAD_ATTR *pAttr); 
int cphThreadSetStackSize(CPH_THREAD_ATTR *pAttr, int stackSize);
int cphThreadStart(CPH_THREADID* threadId, void * (*routine) (void *), void *arg, CPH_THREAD_ATTR *pAttr);
#endif
int cphThreadYield();

#define _CPHTHREAD
#endif
