/*
 * Copyright (c) 2006-2021 Contributors to the Eclipse Foundation
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
#ifndef __FMDTHRD_DEFINED
#define __FMDTHRD_DEFINED

#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <signal.h>


/*
 *  Unix locking
 */

typedef struct {
#ifdef HPUX
    int64_t data[16];       /* Create a large enough object on the correct alignment*/
#else
    int64_t data[8];        /* Create a large enough object on the correct alignment*/
#endif
} fmd_lock_t;

typedef pthread_key_t fmd_threadkey_t;

/**
 * Create a condition wait struture
 * The object is opaque and varies by platform.
 */
typedef struct {
    int64_t data[8]; /**< Create a large enough object on the correct alignment*/
} fmd_cond_t;

/**
 * Define a platform independent thread handle
 */
typedef uintptr_t fmd_threadh_t;
/*
 * Typedef for parameter to start of thread
 */
typedef void * (* fmd_thread_parm_t)(void * parm);


static int fmd_lock(fmd_lock_t * lock) {
    return pthread_mutex_lock((pthread_mutex_t *)lock);
}

static int fmd_unlock(fmd_lock_t * lock) {
    return pthread_mutex_unlock((pthread_mutex_t *)lock);
}

static int fmd_lockinit(fmd_lock_t * lock) {
    return pthread_mutex_init((pthread_mutex_t *)lock, NULL);
}

static int fmd_lockclose(fmd_lock_t * lock) {
    return pthread_mutex_destroy((pthread_mutex_t *)lock);
}


static int fmd_cond_init(fmd_cond_t * cond) {
    return pthread_cond_init((pthread_cond_t *)cond, NULL);
}

static int fmd_cond_signal(fmd_cond_t * cond) {
    return pthread_cond_signal((pthread_cond_t *)cond);
}

static int fmd_cond_broadcast(fmd_cond_t * cond) {
    return pthread_cond_broadcast((pthread_cond_t *)cond);
}

static int fmd_cond_wait(fmd_cond_t * cond, fmd_lock_t * mutex) {
    return pthread_cond_wait((pthread_cond_t *)cond, (pthread_mutex_t *)mutex);
}

static int fmd_cond_destroy(fmd_cond_t * cond) {
    return pthread_cond_destroy((pthread_cond_t *)cond);
}


typedef void * (* thred_arg)(void *);


/*
 * Thread services
 */
static int fmd_startThread(fmd_threadh_t * t, fmd_thread_parm_t addr, void * parm) {
    pthread_t result;
    int rc = pthread_create(&result, NULL, (thred_arg)addr, parm);
    *t = result;
    return rc;
}

static void fmd_endThread(void * retval) {
    pthread_exit(retval);
}

static  int       fmd_joinThread(fmd_threadh_t thread, void ** retvalptr) {
    int rc;
    rc = pthread_join((pthread_t)thread, retvalptr);
    return rc;
}

static int fmd_detachThread(fmd_threadh_t thread) {
    return pthread_detach((pthread_t)thread);
}

/*
 * Set thread name
 */
void fmd_setThreadName(fmd_threadh_t thread, const char * name) {}

static void      fmd_sleep(int micros) {
    struct timespec req , rem ;
    req.tv_sec  = micros / 1000000 ;
    req.tv_nsec = (micros % 1000000) * 1000 ;
    while ( nanosleep(&req, &rem) == -1 )
    {
      if ( errno != EINTR ) break ;
      req.tv_sec  = rem.tv_sec ;
      req.tv_nsec = rem.tv_nsec ;
    }
}

#ifndef fmd_threadSelf
static unsigned int fmd_threadSelf(void) {
    return pthread_self();
}
#endif
#endif

