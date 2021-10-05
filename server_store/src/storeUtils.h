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


#ifndef __ISM_STOREUTILS_DEFINED
#define __ISM_STOREUTILS_DEFINED

#include <pthread.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>

#define ALIGN_REQ (sizeof(double)-1)
#define ALIGNED(x) (((x)+ALIGN_REQ)&(~ALIGN_REQ))

#define MID_INT    0x80000000
#define XltY(x,y)  (((x)-(y))&MID_INT)
#define XgtY(x,y)  (((y)-(x))&MID_INT)
#define XleY(x,y)  (!XgtY(x,y))
#define XgeY(x,y)  (!XltY(x,y))

static inline double su_sysTime(void)
{
  static struct timespec t0;
  struct timespec ts;
  if ( t0.tv_sec + t0.tv_nsec == 0 )
  clock_gettime(CLOCK_MONOTONIC, &t0) ; 
  clock_gettime(CLOCK_MONOTONIC, &ts) ; 
  return (double)(ts.tv_sec-t0.tv_sec) + (double)(ts.tv_nsec-t0.tv_nsec)/1e9 ; 
}

#define SU_NAN  1
#define SU_MIC  1000
#define SU_MIL  1000000
#define SU_SEC  1000000000

static inline void su_sleep(size_t sltm, size_t type)
{
  struct timespec ts, tr ; 
  sltm *= type ; 
  ldiv_t r = ldiv(sltm,SU_SEC) ; 
  ts.tv_sec = r.quot ; 
  ts.tv_nsec= r.rem ; 
  while ( nanosleep(&ts,&tr) < 0 && errno == EINTR )
    ts = tr ; 
}

static inline char *su_strdup(const char *s)
{
  return (s ? ism_common_strdup(ISM_MEM_PROBE(ism_memory_store_misc,1000),s) : NULL) ; 
}

static inline size_t su_strlcpy(char *dst, const char *src, size_t size)
{
  size_t rc=0;
  char *p, *q;
  const char *s;

  if ( src )
  {
    s = src ;
    if ( dst && size > 0 )
    {
      p = dst ;
      q = p + (size - 1) ;
      while ( p<q && *s ) *p++ = *s++ ;
      *p=0;
    }
    while ( *s ) s++ ;
    rc = (s-src);
  }
  else
  if ( dst && size > 0 )
    *dst = 0 ;

  return rc;
}


static inline size_t su_strllen(const char *src, size_t size)
{
  size_t rc=0;
  const char *s, *t;

  if ( src && size > 0 )
  {
    s = src ;
    t = s + size ;
    while ( s<t && *s ) s++ ;
    rc = (s-src);
  }
  return rc;
}
static inline size_t su_strlen(const char *src)
{
  size_t l=1024,m;
  for(;;)
  {
    if ( (m = su_strllen(src,l)) < l ) break ; 
    l += 1024 ; 
  }
  return m ; 
}

/******************************************************************/
#if 0
static inline int su_findLUB(uint64_t target, int lo, int hi, uint64_t value[])
{
  int mid;

  if (     lo > hi        ) return hi+1 ;
  if ( target < value[lo] ) return lo   ;
  if ( target > value[hi] ) return hi+1 ;

  while ( lo<hi )
  {
    mid = (lo+hi)>>1 ;
    if ( target>value[mid] )
      lo = mid+1 ;
    else
      hi = mid ;
  }

  return lo ;
}
#endif
/******************************************************************/

static inline int su_findGLB(uint64_t target, int lo, int hi, uint64_t value[])
{
  int mid;

  if (     lo > hi        ) return lo-1 ;
  if ( target < value[lo] ) return lo-1 ;
  if ( target > value[hi] ) return hi   ;

  while ( lo<hi )
  {
    mid = (lo+hi+1)>>1 ;
    if ( target<value[mid] )
      hi = mid-1;
    else
      lo = mid;
  }

  return hi ;
}

static inline int set_CLOEXEC(int fd)
{
 #ifdef FD_CLOEXEC
  int fl = fcntl(fd,F_GETFD) ; 
  if ( fl < 0 ) return fl ; 
  return fcntl(fd,F_SETFD, (fl|FD_CLOEXEC)) ; 
 #else
  return 0;
 #endif
}

static inline int socket_(int domain, int type, int protocol)
{
 #ifdef SOCK_CLOEXEC
  type |= SOCK_CLOEXEC ; 
  return socket(domain, type, protocol);
 #else
  int fd = socket(domain, type, protocol);
  if ( fd >= 0 ) set_CLOEXEC(fd) ; 
  return fd ; 
 #endif
}

static inline int accept_(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
 #ifdef __USE_GNU
  return accept4(sockfd, addr, addrlen, SOCK_CLOEXEC);
 #else
  int fd = accept(sockfd, addr, addrlen) ; 
  if ( fd >= 0 ) set_CLOEXEC(fd) ; 
  return fd ; 
 #endif
}

static inline int su_mutex_tryLock(pthread_mutex_t *restrict mutex, int waitMilli)
{
  double et = 1e-3*waitMilli + su_sysTime() ;   
  int rc ; 
  for(;;)
  {
    rc = pthread_mutex_trylock(mutex) ; 
    if ( rc == 0 || rc != EBUSY || et < su_sysTime() ) return rc ; 
    su_sleep(1,SU_MIL);
  }
}

#if 0
static inline ism_time_t ism_common_monotonicTimeNanos(void)
{
  struct timespec ts[1];
  clock_gettime(CLOCK_MONOTONIC, ts) ; 
  return (ism_time_t)ts->tv_sec * 1000000000 + ts->tv_nsec ; 
}

static inline int ism_common_cond_init_monotonic(pthread_cond_t *restrict cond) 
{
  int rc;
  pthread_condattr_t attr[1] ; 

  if ( (rc = pthread_condattr_init(attr)) )
    return rc ; 
  if ( (rc = pthread_condattr_setclock(attr, CLOCK_MONOTONIC)) )
    return rc ; 
  if ( (rc = pthread_cond_init(cond, attr)) )
    return rc ; 
  return 0 ; 
}

static inline int ism_common_cond_timedwait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex, const struct timespec *restrict wtime, uint8_t fRelative)
{
  uint64_t nn;
  struct timespec ts[1];
  if ( fRelative )
    clock_gettime(CLOCK_MONOTONIC, ts) ; 
  else
    memset(ts,0,sizeof(ts)) ; 
  ts->tv_sec += wtime->tv_sec ; 
  nn = ts->tv_nsec + wtime->tv_nsec ; 
  while ( nn >= 1000000000 )
  {
    ts->tv_sec++ ; 
    nn -= 1000000000 ; 
  }
  ts->tv_nsec = nn ; 
  return pthread_cond_timedwait(cond, mutex, ts) ; 
}
#endif
#endif
