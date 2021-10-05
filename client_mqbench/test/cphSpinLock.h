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

#ifndef _CPHSPINLOCK

#if defined(WIN32)

#define CACHE_LINE_BYTES 64

/* The CPH_SPINLOCK structure definition */
typedef struct _cphspinlock {
	union {
		char padding [CACHE_LINE_BYTES];
		char lock;
	} lock;
} CPH_SPINLOCK;


#elif defined(AMQ_UNIX)

#include <pthread.h>
#define CPH_SPINLOCK  pthread_mutex_t

#endif

/* Function prototypes */
int cphSpinLock_Init(CPH_SPINLOCK ** ppSLock);
int cphSpinLock_Lock(CPH_SPINLOCK *pSLock);
int cphSpinLock_Unlock(CPH_SPINLOCK *pSLock);
int cphSpinLock_Destroy(CPH_SPINLOCK **ppSLock);

#define _CPHSPINLOCK
#endif
