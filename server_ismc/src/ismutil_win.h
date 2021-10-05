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

/*
 * This should only be included from ismutil.h
 */

/*
 * Windows
 */
#ifdef _WIN32

#pragma warning ( disable : 4996 )
#ifndef XAPI
    #define XAPI extern __declspec(dllexport)
#endif

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0500
#define xUNUSED
#define XINLINE __forecinline

#include <winsock2.h>
#include <process.h>
#include <intrin.h>
#include <errno.h>
#include <time.h>

#define SOEXP ".dll"
#define SOPRE ""
#define dlopen LoadLibrary
#define DLLOAD_OPT
#define dlsym GetProcAddress
#define dlclose FreeLibrary
#define LIBTYPE HMODULE
#define DLERR_FLAGS FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS
#define dlerror_string(str, len) FormatMessage( DLERR_FLAGS, NULL, GetLastError(), 0, (str), (len), 0);
#define strcmpi stricmp
#define snprintf _snprintf
#define strncasecmp strnicmp
#define alloca _alloca
#define strtoll  _strtoi64
#define strtoull _strtoui64
#define ioctl	ioctlsocket
#define strerror_r(x, y, z)	((strerror_s(y, z, x) != 0)?y:"")
#define gmtime_r(x, y) ((gmtime_s(y, x) != 0)?y:NULL)

typedef struct {
    int64_t data[8];  /**< Create a large enough object on the correct alignment*/
} ism_lock_t;

struct timespec {
	time_t  tv_sec;
	uint64_t tv_nsec;
};

#define pthread_t			uint64_t
#define pthread_mutex_t		ism_lock_t
#define pthread_cond_t		CONDITION_VARIABLE
#define pthread_barrier_t	long

extern HANDLE g_eventList[2];
extern HANDLE g_shutdown;
XAPI void ism_setShutdownEvent(HANDLE shutdown);

XAPI int pthread_barrier_wait(pthread_barrier_t * x);

/*
 * Methods for thread local storage
 */
typedef uintptr_t pthread_key_t;
XAPI int  pthread_key_create(uintptr_t * key, void * val);
XAPI int pthread_key_delete(uintptr_t key);
XAPI int pthread_setspecific(uintptr_t key, void * data);
XAPI void * pthread_getspecific(uintptr_t key);

/*
 * Define pthreads locks
 */
enum {SL_FREE = 0, SL_BUSY = EBUSY};
#define sched_yield() SwitchToThread()

#pragma intrinsic ( _InterlockedExchangeAdd, _InterlockedCompareExchange, _InterlockedExchange, _InterlockedDecrement, _InterlockedIncrement, _InterlockedCompareExchangePointer   )


#define pthread_spinlock_t        volatile int
void ism_common_spinwait(pthread_spinlock_t * sl);
#define pthread_spin_init(x, b)   _InterlockedExchange((x),SL_FREE);
#define pthread_spin_lock(x)      if (_InterlockedExchange((x), SL_BUSY)==SL_FREE); else ism_common_spinwait((x));
#define pthread_spin_unlock(x)    _InterlockedExchange((x),SL_FREE);
#define pthread_spin_trylock(x)   _InterlockedExchange((x), SL_BUSY)
#define pthread_spin_destroy(x)

#define PTHREAD_MUTEX_INITIALIZER	CreateMutex(NULL, FALSE, NULL)
#define pthread_mutex_init(l, a)	ism_common_lockinit((l))
#define pthread_mutex_lock(l)		ism_common_lock((l))
#define pthread_mutex_unlock(l)		ism_common_unlock((l))
#define pthread_mutex_trylock(l)	ism_common_lockinit((l))
#define pthread_mutex_destroy(l)	ism_common_lockclose((l))

/*
 * Define condition variables
 */
#define pthread_cond_init(x, y)		InitializeConditionVariable(x)
#define pthread_cond_wait(x, y)		SleepConditionVariableCS ((x), ((CRITICAL_SECTION *)y), INFINITE)
#define pthread_cond_timedwait(x, y, z)		SleepConditionVariableCS ((x), ((CRITICAL_SECTION *)y), 10)
#define pthread_cond_signal(x)		WakeConditionVariable((x))
#define pthread_cond_broadcast(x)	WakeAllConditionVariable((x))
#define pthread_cond_destroy(x)

/*
 * Barriers
 */
#define pthread_barrier_init(x, y, z)	(*x = z)
#define pthread_barrier_destroy(x)
/*
 * Define methods to implement lock functions
 */
XAPI int   ism_common_lock(ism_lock_t * lock);
XAPI int   ism_common_unlock(ism_lock_t * lock);
XAPI int   ism_common_lockinit(ism_lock_t * lock);
XAPI int   ism_common_trylock(ism_lock_t * lock);
XAPI int   ism_common_lockclose(ism_lock_t * lock);

/*
 * Define thread local storage
 */
#define __thread		__declspec(thread)

/*
 * Atomic addition/increment
 */
#define __sync_fetch_and_add(x, y)	((y == 1)?(_InterlockedIncrement(x)):(_InterlockedExchangeAdd(x, y)))

/*
 * Atomic compare and swap
 */
#define __sync_bool_compare_and_swap(x, y, z)	(_InterlockedCompareExchangePointer(x, z, y) == NULL)

#endif
