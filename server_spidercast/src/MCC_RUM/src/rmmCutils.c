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

#ifndef  H_rmmCutils_C
#define  H_rmmCutils_C

#include "rmmCutils.h"


/******************************************************************/

int rmm_snprintf(char *str, size_t size, const char *fmt,...)
{
  int rc ;
  va_list ap ;

  if ( size <= 0 )
    return 0 ;
  va_start(ap,fmt) ;
  rc = vsnprintf(str,size,fmt,ap) ;
  va_end(ap) ;
  if ( rc < 0 || rc > (int)size ) return (int)size ;
  return rc ;
}

/******************************************************************/

size_t rmm_strllen(const char *src, size_t size)
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

// Disabling a specific array bounds warning in GCC due to a known false positive issue.
// This section of code generates a warning for accessing outside array bounds, but after a review
// in October 2024, we concluded it to be a false positive based on reports of such false positives
// for this warning in GCC. The GCC meta-bug tracking this issue can be found here:
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=56456
// GCC version where the false positive was observed: 11.4.1 

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"

size_t rmm_strlcpy(char *dst, const char *src, size_t size)
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

#pragma GCC diagnostic pop

char *rmm_strdup(const char *s)
{
  if ( s )
    return  strdup(s) ; 
  else
    return NULL ; 
}

/******************************************************************/

void tsleep(int sec, int nsec)
{
  struct timespec ts , tr ;

  while ( nsec >= NANO_PER_SEC )
  {
    sec++ ;
    nsec -= NANO_PER_SEC ;
  }
  ts.tv_sec  = sec ;
  ts.tv_nsec = nsec ;
  while ( nanosleep(&ts, &tr) == -1 )
  {
    if ( errno != EINTR )
      break ;
    ts.tv_sec  = tr.tv_sec ;
    ts.tv_nsec = tr.tv_nsec ;
  }
}


/*****************************************************************************************/


#ifndef JNI_CODE

/** !!!!!!!!!!!!!!!!!!!!!! */

int dumpBuff(const char *caller,char *fp,int len,void *buff,int size)
{
  int i,k=0 ;
  uint8_t *p ;
  char *q ;

  p = (uint8_t *)buff ;
  q = (char    *)fp ;
  k+=snprintf(q+k,len-k,"\n_______ %s: Start Buffer Dump ______\n",caller) ;
  for ( i=0 ; i<size ; i++ )
  {
    k+=snprintf(q+k,len-k,"%2.2x ",p[i]) ;
    if ( (i&31) == 31 )
    {
      k+=snprintf(q+k,len-k,"\n") ;
    }
  }
  k+=snprintf(q+k,len-k,"\n_______ %s: End   Buffer Dump ______\n",caller) ;
  return k ;
}


/******************************************************************/

int rmm_atoi(const char *s)
{
  long v;
  v = strtol(s,NULL,10) ; 
  if ( !v )
  v = strtol(s,NULL,16) ; 
  return (int)v ; 
}

/***********************************************************************/
/******************************************************************/

/** !!!!!!!!!!!!!!!!!!!!!! */
int build_log_msg(const llmLogEvent_t *le,char *msg, int msg_size)
{
  int millis, i;
  char timestr[32], *sptr ;
  const char* who;
  i = (le->msg_key/10000);
  switch(i){
    case 2:
      who="RUM";
      break;
    case 5:
      who="DCS";
      break;
    default:
      who="RMM";
  }

  millis = (int)(rmmTime(NULL,timestr)%1000) ;
  for (sptr=timestr ; *sptr ; sptr++ ) if (*sptr == '\n') *sptr = ' ';
  if(le->nparams > 0){
  return snprintf(msg,msg_size,"%.19s.%3.3d| %s.%s %d %s: ",timestr,millis,who,le->module,le->msg_key,le->instanceName);
  }else{
  return snprintf(msg,msg_size,"%.19s.%3.3d| %s.%s %d %s\n",timestr,millis,who,le->module,le->msg_key,le->instanceName);
  }
}


void myLogger(const llmLogEvent_t *le,void *p)
{
  FILE *fp=p ;
  if ( fp )
  {
    char line[1024];
      build_log_msg(le,line,sizeof(line));
    fputs(line,fp) ;
    if(le->nparams > 0){
      fputs(le->event_params[0],fp);
      fputc('\n',fp);
    }
    fflush(fp) ;
  }

}

/******************************************************************/
int  get_OS_info(char *result, int max_len)
{
  int rc=0 ;
  int ws = 8*sizeof(void *) ;
  const char *selOpol , *spinlock;

#ifndef OLD_OS_INFO
  extern const char * fmd_getExecEnv(void);
  const char * tmpstr = fmd_getExecEnv();
#else
  struct utsname sys_info ;
  char tmpstr[128+sizeof(sys_info)] ;
  size_t st = sizeof(tmpstr);

  if ( uname(&sys_info) != -1 )
  {
    snprintf(tmpstr,st," System Info: sysname: %s , nodename: %s , release: %s , version: %s , machine: %s",
                        sys_info.sysname,sys_info.nodename,sys_info.release,sys_info.version,sys_info.machine) ;
  }
  else
  {
    strlcpy(tmpstr," System Info: Failed to obtain System Info!",st) ;
    rc = -1 ;
  }
#endif
 #if USE_POLL
  selOpol = "poll()" ;
 #else
  selOpol = "select()" ;
 #endif
 #if USE_SPINLOCK
  spinlock= "spinlock";
 #else
  spinlock= "mutexlock";
 #endif
  snprintf(result, max_len,"%s, mode: %dbit, Using %s %s", tmpstr, ws, selOpol,spinlock) ;
  return rc ;
}
/******************************************************************/

char *b2h(char *dst,unsigned char *src,int len)
{
  char H[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'} ;
  char *rc = dst ;

  while ( len-- )
  {
    *dst = H[(*src)>>4] ; dst++ ;
    *dst = H[(*src)&15] ; dst++ ;
    src++ ;
  }
  *dst = 0 ;
  return rc ;
}

/***********************************************************************/

unsigned long my_thread_id(void)
{
  unsigned long id;
  id = (unsigned long)pthread_self() ;
  return id ;
}



/***************************************************************************/
uint32_t hash_it(void *target, const void *source, int src_len)
{
  uint32_t h = hash_fnv1a_32(source, src_len, NULL);
  if ( target )
    memcpy(target,&h,4) ;
  return h ;
}

/***********************************************************************/
/*---------------------------------------------------------------------------
 * FNV-1a (Fowler/Noll/Vo) fast and well dispersed hash functions
 * Reference: http://www.isthe.com/chongo/tech/comp/fnv/index.html
 *-------------------------------------------------------------------------*/
uint32_t hash_fnv1a_32(const void *in, int len, void *user)
{
  uint32_t hval = 0x811C9DC5;
  unsigned char *bp = (unsigned char *)in;  /* start of buffer */
  unsigned char *be = bp + len;             /* beyond end of buffer */

  /*
   * FNV-1a hash each octet in the buffer
   */
  while (bp < be)
  {
    /* xor the bottom with the current octet */
    hval ^= (uint32_t)*bp++;
    hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
  }

  return hval;
}

/***********************************************************************/
void rmm_pi_init(PollInfo *pi, int poll_micro, int npoll)
{
  memset(pi,0,sizeof(PollInfo)) ; 
  pi->yield = 1 ; 
  pi->npoll = poll_micro>=0 ? poll_micro : npoll ;
  if ( poll_micro > 0 && poll_micro < 0x7fffffff )
  {
    int i;
    pi->nptime = 1e-6 * poll_micro ; 
    pi->td = sysTime() ; 
    for ( i=0 ; i<10000 ; i++ )
      sched_yield() ; 
    pi->td = (sysTime() - pi->td) / 1e4 ; 
    if ( pi->td < 1e-16 ) pi->td = 1e-16 ; 
    pi->npoll = (int)(.5e0 * pi->nptime / pi->td) ; 
    if ( pi->npoll < 1 ) pi->npoll = 1 ;
    pi->td = 0e0 ; 
    pi->npupd = 500 ; 
  }
}
/***********************************************************************/
void rmm_pi_start(PollInfo *pi)
{
  if ( pi->nptime )
  {
    pi->arm = 1 ; 
    if ( pi->npupd <= 0 && !pi->td )
      pi->td = sysTime() ; 
  }
}
/***********************************************************************/
void rmm_pi_stop(PollInfo *pi)
{
  if ( pi->nptime && pi->arm )
  {
    pi->arm = 0 ; 
    if ( pi->npupd <= 0 )
    {
      int np = pi->npoll ; 
      pi->td = (sysTime() - pi->td) / pi->nptime ; 
      if ( pi->td > 1.15e0 )
           pi->td = 1.15e0 ;
      else
      if ( pi->td <  .85e0 )
           pi->td =  .85e0 ;
      pi->npoll = (int)(pi->npoll/pi->td) ; 
      if ( pi->npoll == np )
      {
        if ( pi->td < .999e0)
          pi->npoll++ ; 
        else
        if ( pi->td > 1.001e0 )
          pi->npoll-- ; 
      }
      if ( pi->npoll < 1 ) pi->npoll = 1 ;
      pi->td = 0e0 ; 
      pi->npupd = 1000 ; 
    }
    else
      pi->npupd-- ; 
  }
}

/***********************************************************************/
int rmm_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, int sec, int nsec)
{
  int rc ; 
  struct timespec ts ;
  double x,y;
   clock_gettime(CLOCK_REALTIME,&ts) ;
   x = (double)ts.tv_sec + (double)ts.tv_nsec/1e9 ; 
  x += (double)sec + (double)nsec/1e9 ; 
  x = modf(x,&y) ; 
  ts.tv_sec = (time_t)y ; 
  ts.tv_nsec= (long)(1e9*x) ; 
  rc = pthread_cond_timedwait(cond, mutex, &ts) ; 
  return rc;
}
/***********************************************************************/

int rmm_cond_init(CondWaitRec *cw, int poll_micro)
{

  if ( MUTEX_INIT(&cw->mutex) != 0 ) return RMM_FAILURE ;
  cw->init |= 1 ;
  if ( MUTEX_INIT(&cw->plock) != 0 ) return RMM_FAILURE ;
  cw->init |= 2 ;
  if ( COND_INIT (&cw->cond ) != 0 ) return RMM_FAILURE ;
  cw->init |= 4 ;

  cw->isOn = 0 ;
  cw->signalPending = 0 ;

  rmm_pi_init(cw->pi, poll_micro, 0) ;

 return RMM_SUCCESS;
}

/***********************************************************************/
int rmm_cond_destroy(CondWaitRec *cw)
{

  if ( (!(cw->init&1) || MUTEX_DESTROY(&cw->mutex) == 0) &&
       (!(cw->init&2) || MUTEX_DESTROY(&cw->plock) == 0) &&
       (!(cw->init&4) || COND_DESTROY(&cw->cond)   == 0)  ) return RMM_SUCCESS;
  else return RMM_FAILURE;

}

/***********************************************************************/
int rmm_cond_wait(CondWaitRec *cw, int maxOn)
{
  int rc=0 ;
  if ( cw->pi->npoll > 0 && !cw->signalPending )
  {
    int nPoll = cw->pi->npoll ; 
    cw->polling=1 ;
    rmm_pi_start(cw->pi) ; 
    while ( nPoll>0 )
    {
      if ( nPoll != 0x7fffffff ) nPoll-- ;
      if ( (rc=MUTEX_LOCK(&cw->plock)) == 0 )
      {
        if ( cw->signalPending )
        {
          cw->polling=0 ;
          cw->signalPending = 0 ;
          MUTEX_UNLOCK(&cw->plock) ;
          return 0 ;
        }
        MUTEX_UNLOCK(&cw->plock) ;
        sched_yield() ;
      }
    }
    rmm_pi_stop(cw->pi) ; 
  }
  if ( (rc=MUTEX_LOCK(&cw->mutex)) == 0 )
  {
    if ( cw->signalPending )
      cw->signalPending = 0 ;
    else
    if ( cw->isOn >= maxOn )
    {
      COND_SIGNAL(&cw->cond) ;
    }
    else
    {
      cw->isOn++ ;
      rc = COND_WAIT(&cw->cond, &cw->mutex) ;
      cw->isOn-- ;
    }
    MUTEX_UNLOCK(&cw->mutex) ;
  }

  return rc ;
}
/***********************************************************************/
int rmm_cond_twait(CondWaitRec *cw, int sec, int nsec, int maxOn)
{
  int rc=0 ;

  if ( (rc=MUTEX_LOCK(&cw->mutex)) == 0 )
  {
    if ( cw->signalPending )
      cw->signalPending = 0 ;
    else
    if ( cw->isOn >= maxOn )
    {
      COND_SIGNAL(&cw->cond) ;
    }
    else
    {
      cw->isOn++ ;
      rc = COND_TWAIT(&cw->cond, &cw->mutex, sec, nsec) ;
      cw->isOn-- ;
    }
    MUTEX_UNLOCK(&cw->mutex) ;
  }

  return rc ;
}

/***********************************************************************/
int rmm_cond_signal(CondWaitRec *cw, int iLock)
{
  int rc=0 ;

  if ( iLock )
  {
    MUTEX_POLLLOCK(&cw->mutex) ;
    if ( cw->isOn )
      COND_SIGNAL(&cw->cond) ;
    else
      if ( !cw->signalPending ) cw->signalPending = 1 ;
    MUTEX_UNLOCK(&cw->mutex) ;
  }
  else
  {
    if ( cw->isOn )
      COND_SIGNAL_NL(&cw->cond) ;
    else
      if ( !cw->signalPending ) cw->signalPending = 1 ;
  }

  return rc ;
}

/***********************************************************************/

int rmm_rwlock_init(rmm_rwlock_t *rw)
{
  memset (rw,0,sizeof(rmm_rwlock_t)) ;

  if ( MUTEX_INIT(&rw->mutex ) != 0 ) return RMM_FAILURE ;
  if ( COND_INIT (&rw->r_cond) != 0 ) return RMM_FAILURE ;
  if ( COND_INIT (&rw->w_cond) != 0 ) return RMM_FAILURE ;
  if ( COND_INIT (&rw->u_cond) != 0 ) return RMM_FAILURE ;

 return RMM_SUCCESS;
}

/***********************************************************************/
int rmm_rwlock_destroy(rmm_rwlock_t *rw)
{
  int rc[4] ;

  rc[0] = MUTEX_DESTROY(&rw->mutex) ;
  rc[1] = COND_DESTROY(&rw->r_cond) ;
  rc[2] = COND_DESTROY(&rw->w_cond) ;
  rc[3] = COND_DESTROY(&rw->u_cond) ;
  if ( !rc[0] && !rc[1] && !rc[2] && !rc[3] )
    return RMM_SUCCESS;
  else return RMM_FAILURE;

}

/***********************************************************************/
int rmm_rwlock_rdlock(rmm_rwlock_t *rw)
{

  if ( MUTEX_LOCK(&rw->mutex) != 0 )
    return RMM_FAILURE;

  while ( rw->w_on || rw->w_wt || rw->u_wt ) /* BEAM suppression: infinite loop */
  {
    rw->r_wt++ ;
    COND_WAIT(&rw->r_cond,&rw->mutex) ;
    rw->r_wt-- ;
  }

  rw->r_on++ ;
  MUTEX_UNLOCK(&rw->mutex) ;

  return RMM_SUCCESS;

}

/***********************************************************************/
int rmm_rwlock_wrlock(rmm_rwlock_t *rw)
{

  if ( MUTEX_LOCK(&rw->mutex) != 0 )
    return RMM_FAILURE;

  while ( rw->w_on || rw->r_on || rw->u_wt )  /* BEAM suppression: infinite loop */
  {
    rw->w_wt++ ;
    COND_WAIT(&rw->w_cond,&rw->mutex) ;
    rw->w_wt-- ;
  }

  rw->w_on++ ;
  rw->nupd++ ;
  MUTEX_UNLOCK(&rw->mutex) ;

  return RMM_SUCCESS;

}

/***********************************************************************/
int rmm_rwlock_tryrdlock(rmm_rwlock_t *rw)
{
  int rc ;

  if ( MUTEX_LOCK(&rw->mutex) != 0 )
    return RMM_FAILURE;

  if ( rw->w_on || rw->w_wt || rw->u_wt )
    rc = RMM_FAILURE ;
  else
  {
    rw->r_on++ ;
    rc = RMM_SUCCESS ;
  }

  MUTEX_UNLOCK(&rw->mutex) ;

  return rc ;

}
/***********************************************************************/
/***********************************************************************/
int rmm_rwlock_rdunlock(rmm_rwlock_t *rw)
{

  if ( MUTEX_LOCK(&rw->mutex) != 0 )
    return RMM_FAILURE;

  rw->r_on-- ;

  if ( rw->u_wt )
   {COND_SIGNAL(&rw->u_cond) ;}
  else
  if ( rw->w_wt )
   {COND_SIGNAL(&rw->w_cond) ;}

  MUTEX_UNLOCK(&rw->mutex) ;

  return RMM_SUCCESS;

}
/***********************************************************************/
int rmm_rwlock_rdunlockif(rmm_rwlock_t *rw)
{
  int rc=RMM_FAILURE ;

  if ( MUTEX_LOCK(&rw->mutex) != 0 )
    return rc ;

  if ( rw->w_wt || rw->u_wt )
  {
    rw->r_on-- ;

    if ( rw->u_wt )
     {COND_SIGNAL(&rw->u_cond) ;}
    else
     {COND_SIGNAL(&rw->w_cond) ;}
    rc = RMM_SUCCESS ;
  }

  MUTEX_UNLOCK(&rw->mutex) ;

  return rc ;

}

/***********************************************************************/
int rmm_rwlock_rdrelock(rmm_rwlock_t *rw)
{

  if ( MUTEX_LOCK(&rw->mutex) != 0 )
    return RMM_FAILURE;

  if ( rw->w_wt || rw->u_wt )
  {
    rw->r_on-- ;

    if ( rw->u_wt )
     {COND_SIGNAL(&rw->u_cond) ;}
    else
     {COND_SIGNAL(&rw->w_cond) ;}

    do
    {
      rw->r_wt++ ;
      COND_WAIT(&rw->r_cond,&rw->mutex) ;
      rw->r_wt-- ;
    } while ( rw->w_on || rw->w_wt || rw->u_wt ) ;

    rw->r_on++ ;
  }

  MUTEX_UNLOCK(&rw->mutex) ;

  return RMM_SUCCESS;

}

/***********************************************************************/
int rmm_rwlock_wrunlock(rmm_rwlock_t *rw)
{

  if ( MUTEX_LOCK(&rw->mutex) != 0 )
    return RMM_FAILURE;

  rw->w_on-- ;

  if ( rw->u_wt )
   {COND_SIGNAL(&rw->u_cond) ;}
  else
  if ( rw->w_wt )
   {COND_SIGNAL(&rw->w_cond) ;}
  else
  if ( rw->r_wt )
   {COND_BROADCAST(&rw->r_cond) ;}

  MUTEX_UNLOCK(&rw->mutex) ;

  return RMM_SUCCESS;

}

/***********************************************************************/
int rmm_rwlock_r2wlock(rmm_rwlock_t *rw)
{

  if ( MUTEX_LOCK(&rw->mutex) != 0 )
    return RMM_FAILURE;

 #ifdef XXX_RMM_R
  if ( rw->u_wt )
  {
    MUTEX_UNLOCK(&rw->mutex) ;
    return RMM_FAILURE;
  }
 #endif

  rw->r_on-- ;
  while ( rw->r_on ) /* BEAM suppression: infinite loop */
  {
    rw->u_wt++ ;
    COND_WAIT(&rw->u_cond,&rw->mutex) ;
    rw->u_wt-- ;
  }
  rw->w_on++ ;
  rw->nupd++ ;

  MUTEX_UNLOCK(&rw->mutex) ;

  return RMM_SUCCESS;

}

/***********************************************************************/
int rmm_rwlock_w2rlock(rmm_rwlock_t *rw)
{

  if ( MUTEX_LOCK(&rw->mutex) != 0 )
    return RMM_FAILURE;

  rw->w_on-- ;
  rw->r_on++ ;
  if ( rw->r_wt )
    COND_BROADCAST(&rw->r_cond) ;

  MUTEX_UNLOCK(&rw->mutex) ;

  return RMM_SUCCESS;


}

/***********************************************************************/
int rmm_rwlock_getnupd(rmm_rwlock_t *rw)
{
  return rw->nupd ;
}

/***********************************************************************/
int rmm_set_thread_priority(pthread_attr_t *attr, int policy, int prio_pct, char *errMsg, int errLen)
{
  int rc = RMM_FAILURE ;

 #if defined(_POSIX_THREAD_PRIORITY_SCHEDULING)
  do
  {
    int min_prio, max_prio ;
    struct sched_param sched_param ;
    pthread_attr_t *pattr_sys ;

    if ( attr == NULL )
      break ;

    pattr_sys = attr ;

    if ( (rc=pthread_attr_getschedparam(pattr_sys , &sched_param)) != 0 )
    {
      snprintf(errMsg,errLen," pthread_attr_getschedparam failed (rc=%d)",rc) ;
      break ;
    }

    if ( policy != SCHED_FIFO && policy != SCHED_RR )
      policy = SCHED_FIFO ;

    if ( (rc=pthread_attr_setschedpolicy(pattr_sys , policy)) != 0 )
    {
      snprintf(errMsg,errLen," pthread_attr_setschedpolicy failed (rc=%d)",rc) ;
      break ;
    }

    if ( (min_prio=sched_get_priority_min(policy)) == -1 )
    {
      snprintf(errMsg,errLen," sched_get_priority_min failed (rc=%d)",rc) ;
      break ;
    }

    if ( (max_prio=sched_get_priority_max(policy)) == -1 )
    {
      snprintf(errMsg,errLen," sched_get_priority_max failed (rc=%d)",rc) ;
      break ;
    }

    sched_param.sched_priority = ((100-prio_pct)*min_prio + prio_pct*max_prio) / 100 ;

    if ( (rc=pthread_attr_setschedparam(pattr_sys , &sched_param)) != 0 )
    {
      snprintf(errMsg,errLen," pthread_attr_setschedparam failed (rc=%d)",rc) ;
      break ;
    }

    if ( (rc=pthread_attr_setinheritsched(pattr_sys , PTHREAD_EXPLICIT_SCHED)) != 0 )
    {
      snprintf(errMsg,errLen," pthread_attr_setinheritsched failed (rc=%d)",rc) ;
      break ;
    }
    rc = RMM_SUCCESS ;
  } while ( 0 ) ;
 #else
  snprintf(errMsg,errLen," rmm_set_thread_priority is not supported on this platform!") ;
 #endif

  return rc ;
}
/***********************************************************************/
int rmm_get_thread_priority(pthread_t thread_id, char *msg, int msgLen)
{
  int rc = RMM_FAILURE ;

  #if defined(_POSIX_THREAD_PRIORITY_SCHEDULING)
   do
   {
     int policy , ss ;
     char pname[16] ;
     struct sched_param sched_param ;
     if ( (rc=pthread_getschedparam(thread_id,&policy,&sched_param)) != 0 )
     {
       snprintf(msg,msgLen," pthread_getschedparam failed (rc=%d)",rc) ;
       break ;
     }
     if ( policy == SCHED_RR )
       strlcpy(pname,"SCHED_RR",16) ;
     else
     if ( policy == SCHED_FIFO )
       strlcpy(pname,"SCHED_FIFO",16) ;
     else
       strlcpy(pname,"SCHED_OTHER",16) ;
     if ( rmm_get_thread_stacksize(thread_id, &ss) == RMM_SUCCESS )
       snprintf(msg,msgLen," My sched policy is %d (%s) , priority is %d , stacksize is %d",policy,pname,sched_param.sched_priority,ss) ;
     else
       snprintf(msg,msgLen," My sched policy is %d (%s) , priority is %d ",policy,pname,sched_param.sched_priority) ;
     rc = RMM_SUCCESS ;
   } while(0) ;
  #else
   snprintf(msg,msgLen," rmm_get_thread_priority is not supported on this platform!") ;
  #endif

  return rc ;
}
/***********************************************************************/

#if USE_AFF
typedef int (* attr_setaffinity_np_t0)(pthread_attr_t * attr,                    const cpu_set_t *cpuset);
typedef int (* attr_setaffinity_np_t1)(pthread_attr_t * attr, size_t cpusetsize, const cpu_set_t *cpuset);
static attr_setaffinity_np_t0 Xpthread_attr_setaffinity_np0=(attr_setaffinity_np_t0)(-1);
static attr_setaffinity_np_t1 Xpthread_attr_setaffinity_np1=(attr_setaffinity_np_t1)(-1);
static pthread_mutex_t  aff_mutex = PTHREAD_MUTEX_INITIALIZER ;
int rmm_set_thread_affinity(pthread_attr_t *attr, int init, rmm_uint64 mask, char *errMsg, int errLen)
{
  int i,rc ;
  cpu_set_t cset[1] ;

  pthread_mutex_lock(&aff_mutex) ; 
  if ( Xpthread_attr_setaffinity_np0 == (attr_setaffinity_np_t0)(-1) )
  {
    void *lib ;
    int ver[3]={0,0,0};
    char gv[64]={0};
    lib = (LIBTYPE) dlopen(NULL DLLOAD_OPT);
    if ( lib )
    {
      Xpthread_attr_setaffinity_np0 = (attr_setaffinity_np_t0)dlsym(lib,"pthread_attr_setaffinity_np");
      Xpthread_attr_setaffinity_np1 = (attr_setaffinity_np_t1)Xpthread_attr_setaffinity_np0;
      dlclose(lib) ;
    }
    else
    {
      Xpthread_attr_setaffinity_np0 = (attr_setaffinity_np_t0)NULL ; 
      Xpthread_attr_setaffinity_np1 = (attr_setaffinity_np_t1)Xpthread_attr_setaffinity_np0;
    }
    confstr(_CS_GNU_LIBC_VERSION, gv, sizeof(gv));
    sscanf(gv,"%s %d.%d.%d",errMsg,ver,ver+1,ver+2);
//  printf("_dbg: %p %p %s %s %d %d %d\n",lib,Xpthread_attr_setaffinity_np0,gv,errMsg,ver[0],ver[1],ver[2]);
    if ( ver[0]>2 || (ver[0]==2 && ver[1]>3) || (ver[0]==2 && ver[1]==3 && ver[2]>3) )
      Xpthread_attr_setaffinity_np0 = NULL ;
    else
      Xpthread_attr_setaffinity_np1 = NULL ;
  }
  pthread_mutex_unlock(&aff_mutex) ; 

  if ( !Xpthread_attr_setaffinity_np0 && !Xpthread_attr_setaffinity_np1 )
  {
    snprintf(errMsg,errLen,"rmm_set_thread_affinity: pthread_attr_setaffinity_np is not available on the current OS");
    return RMM_FAILURE ;
  }

  CPU_ZERO(cset) ;
  if ( init && (rc=pthread_attr_init(attr)) )
  {
    snprintf(errMsg,errLen,"rmm_set_thread_affinity: pthread_attr_init failed, rc=%d %s",rc,rmmErrStr(rc));
    return RMM_FAILURE ;
  }

  for ( i=0 ; i<64 && mask  ; i++ , mask >>= 1 )
  {
    if ( mask&1 )
      CPU_SET(i,cset) ;
  }

  if ( Xpthread_attr_setaffinity_np0 )
    rc = Xpthread_attr_setaffinity_np0(attr, cset) ;
  else
    rc = Xpthread_attr_setaffinity_np1(attr, sizeof(cpu_set_t), cset) ;
  if ( rc )
  {
    snprintf(errMsg,errLen,"rmm_set_thread_affinity: pthread_attr_setaffinity_np failed, rc=%d %s",rc,rmmErrStr(rc));
    return RMM_FAILURE ;
  }

  return RMM_SUCCESS ;
}
#endif /*USE_AFF*/
/***********************************************************************/
int rmm_set_thread_stacksize(pthread_attr_t *attr, int size, char *errMsg, int errLen)
{
  int rc;
  size_t gs, ss , ps ;
  pthread_attr_t *pattr_sys ;

  if ( attr == NULL )
  {
    snprintf(errMsg,errLen,"attr parameter is NULL") ;
    return RMM_FAILURE ;
  }

  pattr_sys = attr ;

  if ( (rc=pthread_attr_getstacksize(pattr_sys, &gs)) != 0 )
  {
    snprintf(errMsg,errLen," pthread_attr_getstacksize failed (rc=%d)",rc) ;
    return RMM_FAILURE ;
  }

  if ( size < 0 )
    size = 0 ;
  ss = (size_t)size ;
  ps = getpagesize() ;
  if ( ps < 1024 )
    ps = 1024 ;
  ss = (ss+ps-1)/ps*ps ;
  if ( ss <= gs )
  {
    snprintf(errMsg,errLen," default stacksize is bigger than requested: %u %u ",gs,ss);
    return RMM_SUCCESS ;
  }

  if ( (rc=pthread_attr_setstacksize(pattr_sys ,  ss)) != 0 )
  {
    snprintf(errMsg,errLen," pthread_attr_setstacksize failed (rc=%d)",rc) ;
    return RMM_FAILURE ;
  }
  return RMM_SUCCESS ;
}


/***********************************************************************/
int rmm_get_thread_stacksize(pthread_t thread_id, int *size)
{
  int rc ;
  size_t gs[1] ;
  pthread_attr_t attr[1] ;
  if ( pthread_getattr_np(pthread_self(), attr) != 0 )
    return RMM_FAILURE ;
  rc = pthread_attr_getstacksize(attr , gs) ; 
  pthread_attr_destroy(attr);
  if ( rc )
    return RMM_FAILURE ;
  size[0] = (int)gs[0] ;
  return RMM_SUCCESS ;
}
/***********************************************************************/
/***********************************************************************/
char *upper(char *str)
{
  char *ip ;
  for ( ip=str ; !isEOL(*ip) ; ip++ ){
    if ( *ip >= 'a' && *ip <= 'z' ) *ip = 'A' + (*ip-'a') ;
  }
  return str ;
}

/***********************************************************************/
char *strip(char *str)
{
  int i,j,k ;

  for ( i=0 ; isspace((int)str[i]) ; i++ ) ;
  for ( j=0, k=0 ; !isEOL(str[i]) ; i++ )
  {
    if ( j<i ) str[j] = str[i] ;
    if ( !isspace((int)str[j]) ) k = j+1 ;
    if ( k ) j++ ;
  }
  str[k] = '\0' ;

  return str ;
}

/******************************************************************/

static double rmmBaseTime=0e0 ;
static double rmmBaseTOD =0e0 ;
  static clockid_t rmm_clock_id=CLOCK_REALTIME ;


void rmmInitTime(void)
{
  struct timeval tv ;
  double curTime ;

  if ( rmmBaseTime == 0e0 )
  {
    #if defined(_POSIX_MONOTONIC_CLOCK) && defined(_SC_MONOTONIC_CLOCK)
     struct timespec ts ;
     double tt[4] ;
     if ( sysconf(_SC_MONOTONIC_CLOCK) > 0 )
     {
       ts.tv_sec = 1 ;
       ts.tv_nsec = 12345678 ;
       clock_gettime(CLOCK_REALTIME,&ts) ;
       tt[0] = (double)ts.tv_sec + (double)ts.tv_nsec*1e-9 ;

       ts.tv_sec = 9 ;
       ts.tv_nsec = 87654321 ;
       clock_gettime(CLOCK_MONOTONIC,&ts) ;
       tt[1] = (double)ts.tv_sec + (double)ts.tv_nsec*1e-9 ;

       ts.tv_sec = 0 ;
       ts.tv_nsec = 10000000 ;
       nanosleep(&ts,NULL) ;

       clock_gettime(CLOCK_REALTIME,&ts) ;
       tt[2] = (double)ts.tv_sec + (double)ts.tv_nsec*1e-9 ;

       clock_gettime(CLOCK_MONOTONIC,&ts) ;
       tt[3] = (double)ts.tv_sec + (double)ts.tv_nsec*1e-9 ;

       if ( fabs((tt[2]-tt[0]) - (tt[3]-tt[1])) < 1e-4 )
         rmm_clock_id = CLOCK_MONOTONIC ;
     }
    #endif

    curTime = sysTime() ;
    gettimeofday(&tv,NULL) ;
    rmmBaseTOD = (double)tv.tv_sec ;
    rmmBaseTime = curTime - (double)tv.tv_usec*1e-6 ;
  }
}
/******************************************************************/

double sysTime(void)
{
  static int init=1 ;
  double curTime ;

  static struct timespec t0 ;
  struct timespec ts ;

  if ( init )
  {
    init = 0 ;
    clock_gettime(rmm_clock_id,&t0) ;
  }
  clock_gettime(rmm_clock_id,&ts) ;
  curTime = (double)(ts.tv_sec-t0.tv_sec) + (double)ts.tv_nsec*1e-9 ;

  return curTime ;
}

double sysTOD(void *userdata)
{
  return (sysTime() - rmmBaseTime + rmmBaseTOD) ;
}

rmm_uint64 rmmTime(time_t *seconds, char *timestr)
{
  rmm_uint64 rc ;
  time_t sec ;
  double curTime ;

  curTime = sysTime() ;
  rc = (rmm_uint64)(1e3*(curTime-rmmBaseTime)) ;

  if ( seconds || timestr )
  {
    sec = (time_t)(rmmBaseTOD + (curTime-rmmBaseTime)) ;
    if ( seconds )
      *seconds = sec ;
    if ( timestr )
      ctime_r(&sec,timestr) ;
  }

  return rc ;
}

#define minmax(v, mn, mx)\
{\
  if ( v < mn ) v = mn ;\
  else\
  if ( v > mx ) v = mx ;\
}\

void TT_del_all_tasks(TimerTasks *tasks)
{
  TaskInfo *t ;
  if ( !tasks )
    return ;

  pthread_mutex_lock(&tasks->visiMutex) ;
  tasks->addTasks[1] = NULL ;
  while ( (t=tasks->addTasks[0]) )
  {
    tasks->addTasks[0] = t->next ;
    free(t) ;
  }
  tasks->Tasks[1] = NULL ;
  while ( (t=tasks->Tasks[0]) )
  {
    tasks->Tasks[0] = t->next ;
    free(t) ;
  }
  tasks->nTasks = 0 ;
  pthread_mutex_unlock(&tasks->visiMutex) ;
}

int TT_add_task(TimerTasks *tasks, const TaskInfo *task)
{
  TaskInfo *t ;
  if ( !(tasks && task) )
    return -1 ;

  if ( (t=(TaskInfo *)malloc(sizeof(TaskInfo))) == NULL )
    return -1 ;
  memcpy(t,task,sizeof(TaskInfo)) ;
  t->next = NULL ;

  pthread_mutex_lock(&tasks->visiMutex) ;
  if ( !tasks->addTasks[1] )
  {
    tasks->addTasks[0] = t ;
    tasks->addTasks[1] = t ;
  }
  else
  {
    tasks->addTasks[1]->next = t ;
    tasks->addTasks[1] = t ;
  }
  tasks->listUpdated = 1 ;
  pthread_mutex_unlock(&tasks->visiMutex) ;
  return 0 ;
}

int TT_del_task(TimerTasks *tasks, const TaskInfo *task)
{
  int i=0, j=0;
  TaskInfo *t ;
  if ( !(tasks && task) )
    return -1 ;

  pthread_mutex_lock(&tasks->visiMutex) ;
  for ( t=tasks->Tasks[0] ; t ; )
  {
    if ( t->taskData    == task->taskData &&
         t->taskCode    == task->taskCode &&
         t->taskParm[0] == task->taskParm[0] )
    {
      t->kill = 1 ;
      tasks->listUpdated = 1 ;
      j++ ; 
    }
    t=t->next ; 
    if ( !t && !i )
    {
      i++ ; 
      t=tasks->addTasks[0] ; 
    }
  }
  pthread_mutex_unlock(&tasks->visiMutex) ;
  return (j?0:-1) ;
}

THREAD_RETURN_TYPE TaskTimer(void *arg)
{
  rmm_uint64 myTime , lastTime ;
  int i,sleeptime ;
  double tb[2], tdiff, counter, Hz , newHz;
  TaskInfo *t, *nt ;
  TimerTasks *myTasks ;

 #if USE_PRCTL
  {
    char tn[16] ; 
    rmm_strlcpy(tn,__FUNCTION__,16);
    prctl( PR_SET_NAME,(uintptr_t)tn);
  }
 #endif

  myTasks = (TimerTasks *)arg ;

  minmax(myTasks->reqSleepTime,1,100) ;

  sleeptime = milli2nano(1) ;
  tb[0] = sysTime() ;
  for ( i=0 ; i<10 ; i++ )
    tsleep(0,sleeptime) ;
  tb[1] = sysTime() ;
  tdiff =1e2*(tb[1] - tb[0]) ;
  minmax(tdiff,1,50) ;

  Hz = 1e3/(myTasks->reqSleepTime + tdiff -1 ) ;
  minmax(Hz,5,1000) ;

  myTasks->actSleepTime = (int)(1000/Hz) ;
  myTasks->updSleepTime = 1 ;
  sleeptime = milli2nano(myTasks->reqSleepTime) ;
  tb[0]     = sysTime() ;
  myTime    = rmmTime(NULL,NULL) ; 
  lastTime  = myTime ;
  counter   = 0 ;

  pthread_mutex_lock(&myTasks->visiMutex) ;
  myTasks->curTime = myTime ; 
  myTasks->threadId = (unsigned long)my_thread_id() ;
  myTasks->ThreadUP = 1 ;
  myTasks->nLoops = 0 ;
  myTasks->listUpdated = 1 ;
  pthread_mutex_unlock(&myTasks->visiMutex) ;

  for(;;)
  {
    myTasks->nLoops++ ;

    pthread_mutex_lock(&myTasks->visiMutex) ;
    if ( myTasks->goDown )
    {
      pthread_mutex_unlock(&myTasks->visiMutex) ;
      break ;
    }
    if ( myTasks->listUpdated )
    {
      myTasks->listUpdated = 0 ;

      for ( t=myTasks->Tasks[0] ; t ; t=nt )
      {
        nt = t->next ;
        if ( t->kill )
        {
          if ( t->prev )
          {
            if ( t->next )
            {
              t->prev->next = t->next ;
              t->next->prev = t->prev ;
            }
            else
            {
              t->prev->next = t->next ;
              myTasks->Tasks[1] = t->prev ;
            }
          }
          else
          {
            if ( t->next )
            {
              myTasks->Tasks[0] = t->next ;
              t->next->prev = t->prev ;
            }
            else
            {
              myTasks->Tasks[0] = t->next ;
              myTasks->Tasks[1] = t->prev ;
            }
          }
          free(t) ;
          myTasks->nTasks-- ;
        }
      }

      for ( t=myTasks->addTasks[0] ; t ; t=nt )
      {
        nt = t->next ;
        if ( t->kill )
        {
          free(t) ;
          continue ; 
        }
        if ( !myTasks->Tasks[1] )
        {
          t->prev = NULL ;
          t->next = NULL ;
          myTasks->Tasks[0] = t ;
          myTasks->Tasks[1] = t ;
        }
        else
        {
          t->prev = myTasks->Tasks[1] ;
          myTasks->Tasks[1] = t ;
          t->prev->next = t ;
          t->next = NULL ;
        }
        myTasks->nTasks++ ;
      }
      myTasks->addTasks[0] = myTasks->addTasks[1] = NULL ;
    }
    myTasks->curTime = myTime ; 
    pthread_mutex_unlock(&myTasks->visiMutex) ;

    for ( t=myTasks->Tasks[0] ; t ; t=t->next )
    {
      if ( myTime >= t->nextTime )
      {
        t->nextTime = t->taskCode(t->nextTime,myTime,t->taskData,t->taskParm) ;
        if ( t->runOnce )
          TT_del_task(myTasks,t) ;
      }
    }

    counter++ ;
    tsleep(0,sleeptime) ;
    myTime = lastTime + lrint(counter*1e3/Hz) ;

    if ( fabs(counter - Hz) < .5e0 )
    {
      tb[1] = sysTime() ;
      tdiff = 1e3*(tb[1] - tb[0]) ;
      minmax(tdiff,750,1250) ;

      newHz  = Hz * (double)(myTime - lastTime) / tdiff ;
      minmax(newHz,5,1000) ;
      if ( fabs(newHz - Hz) > 1e-6 )
      {
        Hz = newHz ; 
        myTasks->actSleepTime = (int)(1000/Hz) ;
        myTasks->updSleepTime++ ;
      }
      lastTime = myTime ;
      tb[0]    = tb[1] ;
      counter  = 1 ;
    }
  }

  TT_del_all_tasks(myTasks) ;
  pthread_mutex_lock(&myTasks->visiMutex) ;
  myTasks->ThreadUP = 0 ;
  pthread_mutex_unlock(&myTasks->visiMutex) ;
  rmm_thread_exit() ;
}


int  get_error_description(int error_code, char* desc, int max_length)
{
  int rc = RMM_SUCCESS ;
  char info[256] ;
  int size = sizeof(info);

  if ( desc == NULL || max_length <=0 )
    return RMM_FAILURE ;

  switch(error_code)
  {
    case RMM_L_E_FATAL_ERROR :
      snprintf(info, size," error_code=%d : FATAL ERROR",error_code); break;
    case RMM_L_E_GENERAL_ERROR :
      snprintf(info, size," error_code=%d : GENERAL ERROR",error_code); break;
    case RMM_L_E_INTERNAL_SOFTWARE_ERROR :
      snprintf(info, size," error_code=%d : An error due to an internal software failure ",error_code); break;
    case RMM_L_E_MEMORY_ALLOCATION_ERROR :
      snprintf(info, size," error_code=%d : An error occurred while trying to allocate memory",error_code); break;
    case RMM_L_E_SOCKET_ERROR :
      snprintf(info, size," error_code=%d : An error related to socket create or access",error_code); break;
    case RMM_L_E_SOCKET_CREATE :
      snprintf(info, size," error_code=%d : An error occurred while creating a socket",error_code); break;
    case RMM_L_E_QUEUE_ERROR :
      snprintf(info, size," error_code=%d : An error related to internal LLM queue handling",error_code); break;
    case RMM_L_E_SEND_FAILURE :
      snprintf(info, size," error_code=%d : Failed to send a packet",error_code); break;
    case RMM_L_E_FILE_NOT_FOUND :
      snprintf(info, size," error_code=%d : A file (such as configuration file) was not found",error_code); break;
    case RMM_L_E_CONFIG_ENTRY :
      snprintf(info, size," error_code=%d : Bad configuration entry",error_code); break;
    case RMM_L_E_BAD_PARAMETER :
      snprintf(info, size," error_code=%d : Bad parameter passed to a function",error_code); break;
    case RMM_L_E_MCAST_INTERF :
      snprintf(info, size," error_code=%d : An error occurred while trying to use the specified network interface",error_code); break;
    case RMM_L_E_TTL :
      snprintf(info, size," error_code=%d : Error in TTL value",error_code); break;
    case RMM_L_E_LOCAL_INTERFACE :
      snprintf(info, size," error_code=%d : An error occurred while trying to set the local interface",error_code); break;
    case RMM_L_E_INTERRUPT :
      snprintf(info, size," error_code=%d : Service interruption error",error_code); break;
    case RMM_L_E_BAD_CONTROL_DATA :
      snprintf(info, size," error_code=%d : Received bad control message",error_code); break;
    case RMM_L_E_BAD_CONTROL_DATA_CREATION :
      snprintf(info, size," error_code=%d : Failed to create control message",error_code); break;
    case RMM_L_E_INTERNAL_LIMIT :
      snprintf(info, size," error_code=%d : Could not perform operation since an internal RMM limit has been exceeded",error_code); break;
    case RMM_L_E_THREAD_ERROR :
      snprintf(info, size," error_code=%d : Thread control error",error_code); break;
    case RMM_L_E_STRUCT_INIT :
      snprintf(info, size," error_code=%d : Error initializing a structure",error_code); break;
    case RMM_L_E_BAD_ADDRESS :
      snprintf(info, size," error_code=%d : Failed to convert given string address to internal form",error_code); break;
    case RMM_L_E_LOAD_LIBRARY :
      snprintf(info, size," error_code=%d : Failed to load dynamic library",error_code); break;
    case RUM_L_E_PORT_BUSY :
      snprintf(info, size," error_code=%d : Failed to bind to listening port",error_code); break;
    case RMM_L_E_INSTANCE_INVALID :
      snprintf(info, size," error_code=%d : Invalid instance structure supplied",error_code); break;
    case RMM_L_E_INSTANCE_CLOSED :
      snprintf(info, size," error_code=%d : Supplied instance structure has been closed",error_code); break;
    case RMM_L_E_TOPIC_INVALID :
      snprintf(info, size," error_code=%d : Invalid topic/queue structure supplied",error_code); break;
    case RMM_L_E_TOPIC_CLOSED :
      snprintf(info, size," error_code=%d : Supplied topic/queue structure has been closed",error_code); break;
    case RMM_L_E_TOO_MANY_INSTANCES :
      snprintf(info, size," error_code=%d : Maximum number of instances have been created",error_code); break;
    case RMM_L_E_TOO_MANY_STREAMS :
      snprintf(info, size," error_code=%d : Maximum number of streams are in service",error_code); break;
    case RUM_L_E_CONN_INVALID :
      snprintf(info, size," error_code=%d : Supplied connection does not exist or no longer valid",error_code); break;
    case RUM_L_E_SHM_OS_ERROR :
      snprintf(info, size," error_code=%d : Shared memory related system call has failed",error_code); break;
    case RMM_L_E_BAD_MSG_PROP :
      snprintf(info, size," error_code=%d : Bad message property",error_code); break;
    case RMM_L_E_BAD_API_VERSION :
      snprintf(info, size," error_code=%d : Bad value of API version",error_code); break;
    case RUM_L_E_LOG_ERROR : 
      snprintf(info, size," error_code=%d : General error in logging utility",error_code); break;
    case RUM_L_E_LOG_INSTANCE_ALREADY_EXISTS : 
      snprintf(info, size," error_code=%d : Logging is already configured for this instance ",error_code); break;
    case RUM_L_E_LOG_INSTANCE_UNKNOWN : 
      snprintf(info, size," error_code=%d : Logging is not configured for this instance",error_code); break;
    case RUM_L_E_LOG_INVALID_CONFIG_PARAM : 
      snprintf(info, size," error_code=%d : Logging configuration parameter is not valid",error_code); break;
    case RUM_L_E_LOG_MAX_NUMBER_OF_COMPONENTS_EXEEDED : 
      snprintf(info, size," error_code=%d : The maximal number of components exceeded in the logging configuration",error_code); break;
    case RUM_L_E_LOG_COMPONENT_NOT_REGISTERED : 
      snprintf(info, size," error_code=%d : Logging is not configured for this component",error_code); break;
    case RUM_L_E_LOG_TOO_MANY_FILTERS_DEFINED : 
      snprintf(info, size," error_code=%d : Too many filters configured for instance logging",error_code); break;
    default :
      snprintf(info, size," error_code=%d : Unrecognized error_code",error_code);
      rc = RMM_FAILURE ;
  }


  strlcpy(desc,info, max_length) ;

  return rc ;
}

int get_param_size(int type, int ind)
{
  switch (type)
  {
    case RMM_PACKET_LOSS :
    case RMM_RECEIVE_QUEUE_TRIMMED :
    case RMM_RELIABILITY_CHANGED :
    case RMM_MEMORY_ALERT_OFF :
    case RMM_MEMORY_ALERT_ON :
    case RMM_MESSAGE_LOSS :
      return (ind==1) ? sizeof(int) : 0 ;
    case RMM_NEW_SOURCE :
      if ( ind == 1 ) return sizeof(rmmStreamInfo) ;
      return 0 ; 
    case RMM_FIRST_MESSAGE :
      return (ind==1) ? sizeof(rmmMessageID_t) : 0 ;
    case RMM_REPAIR_DELAY :
      return (ind==1) ? 3*sizeof(int) : 0 ;
    case RMM_VERSION_CONFLICT :
      if (( ind == 1 ) || (ind == 2)) return sizeof(int) ;
      return 0 ;
    default :
      return 0 ;
  }
}


#ifdef CHECK_ALIGN
static void check_align(void)
{
  int l,m,n;
  char name[]={"check_align"};
  {
    rumConnection s[1];
    l = sizeof(s->rsrv) + (s->rsrv-(char *)s) ;
    m = sizeof(s->rsrv) + 8*4+sizeof(s->local_addr)+sizeof(s->remote_addr);
    n = sizeof(s);
    if ( m-n ) printf("%s_rumConnection: %d %d %d\n",name,l,m,n);
  }
  {
    rumConnectionEvent s[1];
    l = sizeof(s->rsrv) + (s->rsrv-(char *)s) ;
    m = sizeof(s->rsrv) + 4*4+2*sizeof(void *)+sizeof(s->connection_info);
    n = sizeof(s);
    if ( m-n ) printf("%s_rumConnectionEvent: %d %d %d\n",name,l,m,n);
  }
  {
    rumTxQueueParameters s[1];
    l = sizeof(s->rsrv) + (s->rsrv-(char *)s) ;
    m = sizeof(s->rsrv) + 8*4+4*sizeof(void *) +sizeof(s->rum_connection)+sizeof(s->queue_name);
    n = sizeof(s);
    if ( m-n ) printf("%s_rumTxQueueParameters: %d %d %d\n",name,l,m,n);
  }
  {
    rumRxQueueParameters s[1];
    l = sizeof(s->rsrv) + (s->rsrv-(char *)s) ;
    m = sizeof(s->rsrv) + 11*4+10*sizeof(void *) + sizeof(s->queue_name);
    n = sizeof(s);
    if ( m-n ) printf("%s_rumRxQueueParameters: %d %d %d\n",name,l,m,n);
  }
  {
    rumEstablishConnectionParams s[1];
    l = sizeof(s->rsrv) + (s->rsrv-(char *)s) ;
    m = sizeof(s->rsrv) + 10*4+3*sizeof(void *) + sizeof(s->address);
    n = sizeof(s);
    if ( m-n ) printf("%s_rumEstablishConnectionParams: %d %d %d\n",name,l,m,n);
  }
  {
    rumPort s[1];
    l = sizeof(s->rsrv) + (s->rsrv-(char *)s) ;
    m = sizeof(s->rsrv) + 4*4+ sizeof(s->networkInterface);
    n = sizeof(s);
    if ( m-n ) printf("%s_rumPort: %d %d %d\n",name,l,m,n);
  }
  {
    rumConfig s[1];
    l = sizeof(s->rsrv) + (s->rsrv-(char *)s) ;
    m = sizeof(s->rsrv) + 16*4+5*sizeof(void *)+sizeof(s->AdvanceConfigFile)+2*sizeof(s->RxNetworkInterface);
    n = sizeof(s);
    if ( m-n ) printf("%s_rumConfig: %d %d %d\n",name,l,m,n);
  }
  {
    rumTStreamStats s[1];
    l = sizeof(s->rsrv) + (s->rsrv-(char *)s) ;
    m = sizeof(s->rsrv) + 30*4+24+sizeof(s->dest_addr)+sizeof(s->queue_name);
    n = sizeof(s);
    if ( m-n ) printf("%s_rumTStreamStats: %d %d %d\n",name,l,m,n);
  }
  {
    rumRStreamStats s[1];
    l = sizeof(s->rsrv) + (s->rsrv-(char *)s) ;
    m = sizeof(s->rsrv) + 27*4+24+sizeof(s->queue_name);
    n = sizeof(s);
    if ( m-n ) printf("%s_rumRStreamStats: %d %d %d\n",name,l,m,n);
  }
  {
    rumRQueueStats s[1];
    l = sizeof(s->rsrv) + (s->rsrv-(char *)s) ;
    m = sizeof(s->rsrv) + 14*4+2*sizeof(void *) +sizeof(s->queue_name);
    n = sizeof(s);
    if ( m-n ) printf("%s_rumRQueueStats: %d %d %d\n",name,l,m,n);
  }
  {
    rumStats s[1];
    l = sizeof(s->rsrv) + (s->rsrv-(char *)s) ;
    m = sizeof(s->rsrv) + 34*4+4*sizeof(void *) + sizeof(s->local_address);
    n = sizeof(s);
    if ( m-n ) printf("%s_rumStats: %d %d %d\n",name,l,m,n);
  }
  {
    rumConfigStats s[1];
    l = sizeof(s->rsrv) + (s->rsrv-(char *)s) ;
    m = sizeof(s->rsrv) + 58*4+sizeof(void *)+2*sizeof(s->RxNetworkInterface)+sizeof(s->AdvanceConfigFile)+sizeof(s->HostInformation)+sizeof(s->OsInformation)+sizeof(s->RumVersion);
    n = sizeof(s);
    if ( m-n ) printf("%s_rumConfigStats: %d %d %d\n",name,l,m,n);
  }
  {
    rumStreamParameters s[1];
    l = sizeof(s->rsrv) + (s->rsrv-(char *)s) ;
    m = sizeof(s->rsrv) + 6*4+4*sizeof(void *) ;
    n = sizeof(s);
    if ( m-n ) printf("%s_rumStreamParameters: %d %d %d\n",name,l,m,n);
  }
  {
    rumStreamInfo s[1];
    l = sizeof(s->rsrv) + (s->rsrv-(char *)s) ;
    m = sizeof(s->rsrv) + 6*4+4*sizeof(void *) ;
    n = sizeof(s);
    if ( m-n ) printf("%s_rumStreamInfo: %d %d %d\n",name,l,m,n);
  }
  {
    rumRxMessage s[1];
    l = sizeof(s->rsrv) + (s->rsrv-(char *)s) ;
    m = sizeof(s->rsrv) + 5*4+2*sizeof(void *) ;
    n = sizeof(s);
    if ( m-n ) printf("%s_rumRxMessage: %d %d %d\n",name,l,m,n);
  }
  {
    rumTxMessage s[1];
    l = sizeof(s->rsrv) + (s->rsrv-(char *)s) ;
    m = sizeof(s->rsrv) + 8*4+7*sizeof(void *) ;
    n = sizeof(s);
    if ( m-n ) printf("%s_rumTxMessage: %d %d %d\n",name,l,m,n);
  }
  {
    rmmMsgProperty s[1];
    l = sizeof(s->rsrv) + (s->rsrv-(char *)s) ;
    m = sizeof(s->rsrv) + 8*4+2*sizeof(void *) ;
    n = sizeof(s);
    if ( m-n ) printf("%s_rmmMsgProperty: %d %d %d\n",name,l,m,n);
  }
  {
    rmmApplicationMsgFilter s[1];
    l = sizeof(s->rsrv) + (s->rsrv-(char *)s) ;
    m = sizeof(s->rsrv) + 2*4+2*sizeof(void *) ;
    n = sizeof(s);
    if ( m-n ) printf("%s_rmmApplicationMsgFilter: %d %d %d\n",name,l,m,n);
  }
  {
    rmmMsgSelector s[1];
    l = sizeof(s->rsrv) + (s->rsrv-(char *)s) ;
    m = sizeof(s->rsrv) + 4*4+4*sizeof(void *) ;
    n = sizeof(s);
    if ( m-n ) printf("%s_rmmMsgSelector: %d %d %d\n",name,l,m,n);
  }
  {
    rmmPacket s[1];
    l = sizeof(s->rsrv) + (s->rsrv-(char *)s) ;
    m = sizeof(s->rsrv) + 8*4+4*sizeof(void *) + sizeof(s->msgs_info);
    n = sizeof(s);
    if ( m-n ) printf("%s_rmmPacket: %d %d %d\n",name,l,m,n);
  }
  {
    rmmEvent  s[1];
    l = sizeof(s->rsrv) + (s->rsrv-(char *)s) ;
    m = sizeof(s->rsrv) + 6*4+2*sizeof(void *) + sizeof(s->source_addr)+sizeof(s->topic_name);
    n = sizeof(s);
    if ( m-n ) printf("%s_rmmEvent: %d %d %d\n",name,l,m,n);
  }
  {
    rmmLogEvent s[1];
    l = sizeof(s->rsrv) + (s->rsrv-(char *)s) ;
    m = sizeof(s->rsrv) + 6*4+4*sizeof(void *) ;
    n = sizeof(s);
    if ( m-n ) printf("%s_rmmLogEvent: %d %d %d\n",name,l,m,n);
  }
  {
    rmmLatencyMonParams s[1];
    l = sizeof(s->rsrv) + (s->rsrv-(char *)s) ;
    m = sizeof(s->rsrv) + 8*4+2*sizeof(void *) ;
    n = sizeof(s);
    if ( m-n ) printf("%s_rmmLatencyMonParams: %d %d %d\n",name,l,m,n);
  }
  {
    rmmHistogram s[1];
    l = sizeof(s->rsrv) + (s->rsrv-(char *)s) ;
    m = sizeof(s->rsrv) + 24*4+2*sizeof(void *);
    n = sizeof(s);
    if ( m-n ) printf("%s_rmmHistogram: %d %d %d\n",name,l,m,n);
  }
}
#endif

static int setLogConfig(const char* instanceName, int filterID, int logLevel, llm_on_log_event_t logger,
                 void* user, int update, int* ec){
    llmInstanceLogCfg_t logConfig;
    int errorCode;
    memset(&logConfig,0,sizeof(llmInstanceLogCfg_t));
    logConfig.filterID = filterID;
    logConfig.instanceName = rmm_strdup(instanceName);
    if(logConfig.instanceName == NULL) {
        *ec = RMM_L_E_MEMORY_ALLOCATION_ERROR;
        return RMM_FAILURE;
    }
    logConfig.allowedLogLevelDefault = logLevel;
    logConfig.onLogEvent = (llm_on_log_event_t)logger;
    logConfig.user = user;
    if(llmSetInstanceLogConfig(&logConfig,update,&errorCode) != 0){
        if(ec != NULL){
            switch(errorCode){
                case INSTANCE_ALREADY_EXISTS:
                    *ec = RUM_L_E_LOG_INSTANCE_ALREADY_EXISTS;
                    break;
                case INSTANCE_UNKNOWN:
                    *ec = RUM_L_E_LOG_INSTANCE_UNKNOWN;
                    break;
                case NOT_ENOUGH_MEMORY:
                    *ec = RUM_L_E_MEMORY_ALLOCATION_ERROR;
                    break;
                case INVALID_CONFIG_PARAM:
                    *ec = RUM_L_E_LOG_INVALID_CONFIG_PARAM;
                    break;
                case MAX_NUMBER_OF_COMPONENTS_EXEEDED:
                    *ec = RUM_L_E_LOG_MAX_NUMBER_OF_COMPONENTS_EXEEDED;
                    break;
                case COMPONENT_NOT_REGISTERED:
                    *ec = RUM_L_E_LOG_COMPONENT_NOT_REGISTERED;
                    break;
                case TOO_MANY_FILTERS_DEFINED:
                    *ec = RUM_L_E_LOG_TOO_MANY_FILTERS_DEFINED;
                    break;
                default:
                    *ec = RUM_L_E_LOG_ERROR;
                    break;
            }
        }
        free(logConfig.instanceName);
        return RMM_FAILURE;
    }
    free(logConfig.instanceName);
    return RMM_SUCCESS;
}
static int unsetLogConfig(const char* instanceName, int filterID){
    return llmRemoveInstanceLogConfig(instanceName,filterID,NULL);
}


#endif

#endif
