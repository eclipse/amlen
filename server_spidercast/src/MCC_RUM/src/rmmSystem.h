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

#ifndef  H_rmmSystem_H
#define  H_rmmSystem_H

#ifndef xUNUSED
    #if defined(__GNUC__)
        #define xUNUSED __attribute__ (( unused ))
    #else
        #define xUNUSED
    #endif
#endif

#ifndef BUILD_DATE
#define BUILD_DATE __DATE__
#endif

#ifndef BUILD_TIME
#define BUILD_TIME __TIME__
#endif

#ifndef BUILD_ID
#define BUILD_ID PRIVATE_BUILD
#endif

/* BUILD_ID defined in makefile; version_string must have ":" in it */
#define XSTR(s) STR(s)
#define STR(s) #s

xUNUSED static const char * llm_static_copyright_string = "\n"
        "Copyright (c) 2014-2021 Contributors to the Eclipse Foundation\n"
        "See the NOTICE file(s) distributed with this work for additional\n"
        "information regarding copyright ownership.\n\n"
        "This program and the accompanying materials are made available under the\n"
        "terms of the Eclipse Public License 2.0 which is available at\n"
        "http://www.eclipse.org/legal/epl-2.0\n"
        "SPDX-License-Identifier: EPL-2.0\n\n";

#if !defined(RUM_WINDOWS) && !defined(RUM_UNIX)
 #if (defined(WIN32) || defined(_WIN32))
  #define RUM_WINDOWS
 #else
  #define RUM_UNIX
 #endif
#endif


#ifndef RMMAPI_DLL
#       define RMMAPI_DLL
#endif


#if defined(SUN) && !defined(OS_SunOS)
 #define OS_SunOS
#endif
#if defined(AIX) && !defined(OS_AIX)
 #define OS_AIX
#endif

#ifdef OLD_OS_INFO
 #define RMM_OS_TYPE "Linux"
#else
  extern const char * fmd_getOsName(void);
  #define RMM_OS_TYPE fmd_getOsName()
#endif
#if defined(RMM_WINDOWS) || defined(RUM_WINDOWS)
 #define RMM_OS_TYPE "Windows"
#endif

#if defined(OS_Linux) && ENABLE_IB
 #define USE_IB 1
#else
 #define USE_IB 0
#endif

#if defined(OS_Linux) && defined(ENABLE_SSL)
 #define USE_SSL 1
#else
 #define USE_SSL 0
#endif

 #define USE_IBEL 0
 
 #define USE_PRCTL 1
#if USE_PRCTL
 #include <sys/prctl.h>
#endif

#if ENABLE_SHM && (defined(OS_Linux) || defined(OS_SunOS) || defined(OS_AIX))
 #define USE_SHM 1
  #define USE_FCNTL_LOCK 0
#else
  #define USE_SHM 0
#endif

 #define USE_AFF 1

#if defined(RMM_UNIX) && defined(ENABLE_POLL)
 #define USE_POLL 1
 #define llmFD_ZERO(x)
 #define llmFD_CLR(x,y)
 #define llmFD_SET(x,y)
 #define llmFD_ISSET(x,y)
#else
 #define USE_POLL 0
 #define llmFD_ZERO(x)    FD_ZERO(x)
 #define llmFD_CLR(x,y)   FD_CLR(x,y)
 #define llmFD_SET(x,y)   FD_SET(x,y)
 #define llmFD_ISSET(x,y) FD_ISSET(x,y)
#endif


#if defined(__GNUC__) || (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L)
 #define RMM_INLINE inline
#else
 #define RMM_INLINE
#endif
#ifndef RMM_INLINE
 #define RMM_INLINE
#endif

/*****************  WINDOWS (SYSTEM) *****************/


/*****************  UNIX (SYSTEM) *****************/

#include        <stddef.h>
#include        <sched.h>
#include        <sys/types.h>   /* basic system data types                */
#ifdef OS_Linuxx
#include       <nptl/pthread.h>
#else
#include        <pthread.h>
#endif
#include        <stdlib.h>
#include        <stdarg.h>
#include        <stdint.h>
#include        <stdio.h>
#include        <malloc.h>
#include        <ctype.h>
#include        <string.h>
#include        <sys/utsname.h>
#include        <math.h>
#include        <sys/socket.h>  /* basic socket definitions               */
#include        <sys/time.h>    /* timeval{} for select()                 */
#include        <sys/timeb.h>   /* timeb for ftime                        */
#include        <time.h>        /* timespec{} for pselect()               */
#include        <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include        <netinet/ip6.h>  /* sockaddr_in{} and other Internet defns */
#include        <sys/ioctl.h>
#include        <netinet/tcp.h>  /* TCP_xxx defines */
#include        <sys/poll.h>
#include        <sys/param.h>   /* ALIGN macro for CMSG_NXTHDR() macro */
#include        <errno.h>
#include        <fcntl.h>       /* for nonblocking                        */
#include        <netdb.h>
#include        <signal.h>
#include        <sys/stat.h>    /* for S_xxx file mode constants          */
#include        <sys/uio.h>     /* for iovec{} and readv/writev           */
#include        <unistd.h>
#include        <sys/wait.h>
#include        <arpa/inet.h>   /* inet(3) functions                      */
#include        <net/if.h>
#ifndef SIOCGIFCONF
#include       <sys/sockio.h>
#endif
#include       <ifaddrs.h>

#if !defined(OS_HP_UX) && (defined(IP_RECVDSTADDR) || defined(IP_PKTINFO))
#define HAVE_MSGHDR_MSG_CONTROL
#endif

#define  SOCKADDR_IN sockaddr_in
#define  SOCKET      int
#define  ULONG      uint32_t
#define  INVALID_SOCKET  -1
#define  SOCKET_ERROR    -1

#define rmm_select(np1,rfds,wfds,efds,tv) select(np1,rfds,wfds,efds,tv)

#define rmmErrno  errno
#define rmm_poptval void *
#define Sleep(x)  tsleep((x)/1000, 1000000*((x)%1000))
#ifdef OS_AIXx
#define sched_yield() tsleep(0,1000)
#endif

/****************************************/
/* Unix based systems */

    #include <dlfcn.h>
    #define RMM_SOEXP ".so"
    #define RMM_SOPRE ""
    #define dlerror_string(str, len) strlcpy((str), dlerror(), (len));
    #if defined(SOLARIS)
        #define DLLOAD_OPT ,RTLD_LAZY | RTLD_PARENT
    #else
        #define DLLOAD_OPT ,RTLD_LAZY
    #endif
    #define LIBTYPE void *

#define DEFSYM(ret, name, parm) typedef ret (* t_##name)parm; static t_##name X##name
#define GETSYM(lib, name) (X##name = (t_##name) dlsym(lib, #name))

/****************************************/

#ifndef SHORT_SIZE
#define SHORT_SIZE 2
#endif

#ifndef INT_SIZE
#define INT_SIZE 4
#endif

#ifndef LONG_SIZE
#define LONG_SIZE 8
#endif

#ifndef UNION_LI
#define UNION_LI
typedef union {
        double   d ;
        uint64_t l ;
        uint32_t i[2] ;
} uli ;
#endif

#ifndef BIG_ENDIAN
#define BIG_ENDIAN    4321
#endif

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1234
#endif

#ifndef BYTE_ORDER
 #if   defined(__BYTE_ORDER)
  #define BYTE_ORDER __BYTE_ORDER
 #elif defined(G_BYTE_ORDER)
  #define BYTE_ORDER G_BYTE_ORDER
 #elif defined(_LITTLE_ENDIAN)
  #define BYTE_ORDER LITTLE_ENDIAN
 #elif defined(_BIG_ENDIAN)
  #define BYTE_ORDER BIG_ENDIAN
 #elif defined(hpux) || defined(_hpux) || defined(__hpux)
  #define BYTE_ORDER BIG_ENDIAN
 #elif defined(__x86) || defined(i386) || defined(__i386) || defined(__i386__) || defined(_M_IX86)
  #define BYTE_ORDER LITTLE_ENDIAN
 #elif defined(__ia64) || defined(__ia64__) || defined(__x86_64) || defined(__amd64)
  #define BYTE_ORDER LITTLE_ENDIAN
 #elif defined(__sparc__) || defined(__sparc)
  #define BYTE_ORDER BIG_ENDIAN
 #else
  #define BYTE_ORDER 0
 #endif
#endif

static int rmm_is_big ;

#if BYTE_ORDER == LITTLE_ENDIAN
#endif

#ifndef htonll
  #if BYTE_ORDER == BIG_ENDIAN
    #define htonll(x) (x)
  #elif BYTE_ORDER == LITTLE_ENDIAN
      #include <byteswap.h>
      #define htonll(x) bswap_64(x)
  #else
    static inline uint64_t htonll(x) {if(rmm_is_big) return (x); return (htonl((uint32_t)(x>>32)) | ((uint64_t)htonl((uint32_t)(x)))<<32);}
  #endif
#endif

#ifndef ntohll
#define ntohll(x)  htonll(x)
#endif

#define NgetChar(ptr,val)         {val=ptr[0];ptr++;}
#define NgetShort(ptr,val) {uint16_t netval;memcpy(&netval,ptr,SHORT_SIZE);val=ntohs(netval);ptr+=SHORT_SIZE;}
#define NgetInt(ptr,val)   {uint32_t netval;memcpy(&netval,ptr,INT_SIZE);val=ntohl(netval);ptr+=INT_SIZE;}
#define NgetLong(ptr,val)  {uli netval;memcpy(&netval.l,ptr,LONG_SIZE);val.l=ntohll(netval.l);ptr+=LONG_SIZE;}
#define NgetLongX(ptr,val) {uint64_t netval;memcpy(&netval,ptr,LONG_SIZE);val=ntohll(netval);ptr+=LONG_SIZE;}
#define NgetDouble(ptr,val) NgetLong(ptr,val)

#define NputChar(ptr,val)         {ptr[0]=val;ptr++;}
#define NputShort(ptr,val) {uint16_t netval;netval=htons(val);memcpy(ptr,&netval,SHORT_SIZE);ptr+=SHORT_SIZE;}
#define NputInt(ptr,val)   {uint32_t netval;netval=htonl(val);memcpy(ptr,&netval,INT_SIZE);ptr+=INT_SIZE;}
#define NputLong(ptr,val)  {uli netval;netval.l=htonll(val.l);memcpy(ptr,&netval.l,LONG_SIZE);ptr+=LONG_SIZE;}
#define NputLongX(ptr,val) {uint64_t netval;netval=htonll(val);memcpy(ptr,&netval,LONG_SIZE);ptr+=LONG_SIZE;}
#define NputDouble(ptr,val) NputLong(ptr,val)

typedef uint32_t pgm_seq ;

#define snprintf rmm_snprintf
#define strlcpy  rmm_strlcpy
#define strllen  rmm_strllen

#define MSG_KEY(key)         (MSG_KEY_PREFIX+(key))

#include "llmLogUtil.h"
#define LLU_VALUE(x) ((long long unsigned int)((uintptr_t)(x)))
#define LLD_VALUE(x) ((long long int)((intptr_t)(x)))
#define UINT_64_TO_LLU(x) ((long long unsigned int)(x))
#define INT_64_TO_LLD(x) ((long long int)(x))

#define SET_ERROR_CODE( pErrorCode, errorCode ) if ((pErrorCode) != NULL ) { *(pErrorCode) = errorCode; }

#define LOG_MEM_ERR(tch,methodName,size) {                                          \
    llmSimpleTraceInvoke(tch, LLM_LOGLEV_ERROR, MSG_KEY(3008), "%s%d%d",    \
        "The allocation of memory in the {0}({1}) method failed. The requested size is {2}.",        \
        methodName,__LINE__,((int)(size)));                                                 \
}
#define LOG_METHOD_ENTRY()  { \
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_XTRACE,MSG_KEY(9001),"%s",     \
    "{0}(): Entry",methodName); \
}

#define LOG_METHOD_EXIT()   { \
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_XTRACE,MSG_KEY(9002),"%s%d",       \
    "{0}(): Exit:{1}",methodName,__LINE__); \
}

#define LOG_METHOD_EXIT_RC(x)   { \
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_XTRACE,MSG_KEY(9003),"%s%d%d",       \
    "{0}(): Exit({2}):{1}",methodName,__LINE__,(x)); \
}

#define LOG_STRUCT_NOT_INIT(structName) { \
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3002),"%s%s",        \
    "The {0} structure was not initialized properly before calling the {1} method.",structName,methodName); \
}
#define LOG_NULL_PARAM(param)   { \
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3012),"%s%s",                                \
    "The {0} parameter passed to the {1} method is not valid because the value is NULL.",param, methodName);    \
}

#define LOG_FAIL_CALLBACK_REPLACE(callback) { \
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3004),"%s",      \
    "The callback ({0}) could not be changed because it was previously set to NULL.",callback);   \
}

#define IS_IB(x)   (((x)==RMM_MULTI_PROTOCOL)?0:((x)&(RMM_PGM_IB|RMM_PGM_IB_IP|RMM_TCP_IB)))
#define IS_TCP(x)  (((x)==RMM_MULTI_PROTOCOL)?0:((x)&(RMM_TCP|RMM_TCP_IB)))
#define IB_TYPE(x) (((x)==RMM_MULTI_PROTOCOL)?0:(((x)==RMM_PGM_IB)+2*((x)==RMM_PGM_IB_IP)+4*((x)==RMM_TCP_IB)))


#define THREAD_ID(thandle)  (LLU_VALUE(thandle))


typedef struct
{
  int llm_api_version ; 
  int llm_api_init_sig ; 
} llmCommonInitStruct ; 

#if ENABLE_SPINLOCK 
 #define USE_SPINLOCK 1
#else
 #define USE_SPINLOCK 0
#endif

#if USE_SPINLOCK
 typedef struct {pthread_cond_t c[1] ; pthread_mutex_t m[1];} cond_mutex_t;
 #define MUTEX_TYPE             pthread_spinlock_t
 #define COND_TYPE              cond_mutex_t
 #define MUTEX_INIT(x)          pthread_spin_init(x,PTHREAD_PROCESS_PRIVATE)
 #define MUTEX_INIT2(x,y)       pthread_spin_init(x,y)
 #define COND_INIT(x)          (pthread_cond_init((x)->c,NULL),pthread_mutex_init((x)->m,NULL))
 #define COND_INIT2(x,y,z)     (pthread_cond_init((x)->c,y),pthread_mutex_init((x)->m,z))
 #define MUTEX_DESTROY(x)       pthread_spin_destroy(x)
 #define COND_DESTROY(x)       (pthread_cond_destroy((x)->c),pthread_mutex_destroy((x)->m))
// #define MUTEX_LOCK(x)          pthread_spin_lock(x)
// #define MUTEX_LOCK(x)          while(pthread_spin_trylock(x)) (sched_yield(),0)
 static RMM_INLINE int MUTEX_LOCK(MUTEX_TYPE *x) {while(pthread_spin_trylock(x)) sched_yield(); return 0;}
 #define MUTEX_POLLLOCK(x)      while(pthread_spin_trylock(x)) sched_yield()
 #define MUTEX_TRYLOCK(x)       pthread_spin_trylock(x)
 #define MUTEX_UNLOCK(x)        pthread_spin_unlock(x)
 static RMM_INLINE int COND_WAIT(COND_TYPE *x,MUTEX_TYPE *y) {int rc;pthread_mutex_lock((x)->m);pthread_spin_unlock(y);rc=pthread_cond_wait((x)->c,(x)->m);pthread_mutex_unlock((x)->m);while(pthread_spin_trylock(y)) sched_yield();return rc;}
 static RMM_INLINE int COND_TWAIT(COND_TYPE *x,MUTEX_TYPE *y,int s,int n) {int rc;pthread_mutex_lock((x)->m);pthread_spin_unlock(y);rc=rmm_cond_timedwait((x)->c,(x)->m,s,n);pthread_mutex_unlock((x)->m);while(pthread_spin_trylock(y)) sched_yield();return rc;}
 #define COND_SIGNAL(x)        (pthread_mutex_lock((x)->m),pthread_cond_signal((x)->c),pthread_mutex_unlock((x)->m))
 #define COND_SIGNAL_NL(x)      pthread_cond_signal((x)->c)
 #define COND_BROADCAST(x)     (pthread_mutex_lock((x)->m),pthread_cond_broadcast((x)->c),pthread_mutex_unlock((x)->m))
#else                         
 #define MUTEX_TYPE             pthread_mutex_t
 #define COND_TYPE              pthread_cond_t
 #define MUTEX_INIT(x)          pthread_mutex_init(x,NULL)
 #define MUTEX_INIT2(x,y)       pthread_mutex_init(x,y)
 #define COND_INIT(x)           pthread_cond_init(x,NULL)
 #define COND_INIT2(x,y,z)      pthread_cond_init(x,y)
 #define MUTEX_DESTROY(x)       pthread_mutex_destroy(x)
 #define COND_DESTROY(x)        pthread_cond_destroy(x)
 #define MUTEX_LOCK(x)          pthread_mutex_lock(x)
 #define MUTEX_POLLLOCK(x)      pthread_mutex_lock(x)
 #define MUTEX_TRYLOCK(x)       pthread_mutex_trylock(x)
 #define MUTEX_UNLOCK(x)        pthread_mutex_unlock(x)
 #define COND_WAIT(x,y)         pthread_cond_wait(x,y)
 #define COND_TWAIT(x,y,s,n)    rmm_cond_timedwait(x,y,s,n)
 #define COND_SIGNAL(x)         pthread_cond_signal(x)
 #define COND_SIGNAL_NL(x)      pthread_cond_signal(x)
 #define COND_BROADCAST(x)      pthread_cond_broadcast(x)
#endif

#define PGM_VERSION           0
#define TURBO_FLOW_VERSION    10
#define MSG_PROP_VERSION      11

#define RMM_API_VERSION    "C" XSTR(VERSION_ID) " (" XSTR(BUILD_ID) ")"
#define RMM_or_RUM           "RUM"
#define MSG_KEY_PREFIX       20000

#endif
