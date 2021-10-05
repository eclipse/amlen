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

#ifndef  H_rmmCutils_H
#define  H_rmmCutils_H


#define rmmMin(X, Y)  ((X) < (Y) ? (X) : (Y))
#define rmmMax(X, Y)  ((X) > (Y) ? (X) : (Y))

static int rmm_snprintf(char *str, size_t size, const char *fmt,...);
static size_t rmm_strllen(const char *src, size_t size);
static size_t rmm_strlcpy(char *dst, const char *src, size_t size);
static char *rmm_strdup(const char *s);
static void tsleep(int sec, int nsec) ;

#ifndef JNI_CODE

static int rmm_atoi(const char *s);
static int dumpBuff(const char *caller,char *fp,int len, void *buff,int size) ;

#ifdef OS_Linuxx
  #define my_gettid gettid
#else
  #define my_gettid my_thread_id
#endif

typedef struct
{
  double  nptime ; 
  double  td ; 
  int     npupd ; 
  int     npoll  ; 
  int     yield  ; 
  int     arm ; 
} PollInfo ;

#ifndef COND_WAIT_REC
#define COND_WAIT_REC
typedef struct {
        MUTEX_TYPE       mutex  ;
        MUTEX_TYPE       plock  ;
        COND_TYPE        cond   ;
        int              isOn   ;
        int              polling ;
        int              signalPending ;
        int              init ;
        PollInfo         pi[1] ; 
        struct timespec  ts ;
}  CondWaitRec ;

#endif

#ifndef RMM_RWLOCK_REC
#define RMM_RWLOCK_REC
typedef struct
{
        MUTEX_TYPE       mutex   ;
        COND_TYPE        r_cond  ;
        COND_TYPE        w_cond  ;
        COND_TYPE        u_cond  ;
        int              r_on ;
        int              w_on ;
        int              r_wt ;
        int              w_wt ;
        int              u_wt ;
        int              nupd ;
}  rmm_rwlock_t;

#endif

#ifndef UNION_ISC
#define UNION_ISC
typedef union {
        unsigned int   i ;
        unsigned short s[2] ;
        unsigned char  c[4] ;
} isc ;
#endif

typedef rmm_uint64 (*pt2taskCode)(rmm_uint64, rmm_uint64, void *, int *);
typedef struct TaskInfo_
{
  rmm_uint64       nextTime ;
  int              taskParm[4] ;
  int              runOnce ;
  int              kill ;
  void *           taskData ;
  pt2taskCode      taskCode ;
  struct TaskInfo_ *prev , *next ;
} TaskInfo ;

typedef struct
{
 unsigned long    threadId;
  int             ThreadUP ;
  int             nLoops ;
  int             nTasks ;
  int             goDown ;
  int             listUpdated ;
  int             reqSleepTime ;
  int             actSleepTime ;
  int             updSleepTime ;
  pthread_mutex_t visiMutex ;
  TaskInfo       *Tasks[2] ;
  TaskInfo       *addTasks[2] ;
  rmm_uint64      curTime ;
} TimerTasks ;

static int TT_add_task(TimerTasks *tasks, const TaskInfo *task) ;
static int TT_del_task(TimerTasks *tasks, const TaskInfo *task) ;
static void TT_del_all_tasks(TimerTasks *tasks) ;


static int build_log_msg(const llmLogEvent_t *le,char *msg, int msg_size);
static void myLogger(const llmLogEvent_t *log_event,void *p) ;

static int  get_OS_info(char *result, int max_len) ;
static THREAD_RETURN_TYPE TaskTimer(void *arg) ;
static char *b2h(char *dst, unsigned char *src , int len) ;
static unsigned long my_thread_id(void);
static uint32_t hash_it(void *target, const void *source, int src_len) ;
static uint32_t hash_fnv1a_32(const void *in, int len, void *user) ;
static char *upper(char *str) ;
static char *strip(char *str) ;

static void rmm_pi_init(PollInfo *pi, int poll_micro, int npoll);
static void rmm_pi_start(PollInfo *pi);
static void rmm_pi_stop(PollInfo *pi);

static int rmm_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, int sec, int nsec);

static int  rmm_cond_init(CondWaitRec *cw, int poll_micro);
static int  rmm_cond_destroy(CondWaitRec *cw) ;
static int  rmm_cond_wait(CondWaitRec *cw, int maxOn) ;
static int  rmm_cond_twait(CondWaitRec *cw, int sec, int nsec, int maxOn) ;
static int  rmm_cond_signal(CondWaitRec *cw, int iLock) ;

static int rmm_rwlock_init(rmm_rwlock_t *rw) ;
static int rmm_rwlock_destroy(rmm_rwlock_t *rw) ;
static int rmm_rwlock_rdlock(rmm_rwlock_t *rw) ;
static int rmm_rwlock_wrlock(rmm_rwlock_t *rw) ;
static int rmm_rwlock_tryrdlock(rmm_rwlock_t *rw) ;
static int rmm_rwlock_rdunlock(rmm_rwlock_t *rw) ;
static int rmm_rwlock_wrunlock(rmm_rwlock_t *rw) ;
static int rmm_rwlock_rdrelock(rmm_rwlock_t *rw) ;
static int rmm_rwlock_rdunlockif(rmm_rwlock_t *rw) ;
static int rmm_rwlock_r2wlock(rmm_rwlock_t *rw) ;
static int rmm_rwlock_w2rlock(rmm_rwlock_t *rw) ;
static int rmm_rwlock_getnupd(rmm_rwlock_t *rw) ;

static int rmm_set_thread_priority(pthread_attr_t *attr, int policy, int prio_pct, char *errMsg, int errLen);
static int rmm_get_thread_priority(pthread_t thread_id, char *msg, int msgLen);
static int rmm_set_thread_stacksize(pthread_attr_t *attr, int size, char *errMsg, int errLen);
static int rmm_get_thread_stacksize(pthread_t thread_id, int *size);
#if USE_AFF
static int rmm_set_thread_affinity(pthread_attr_t *attr, int init, rmm_uint64 mask, char *errMsg, int errLen);
#endif

static void rmmInitTime(void) ;
static rmm_uint64 rmmTime(time_t *seconds, char *timestr) ;
static double sysTime(void) ;

static double sysTOD(void * userdata) ;

#define isEOL(c) ((c)=='\0'||(c)=='\r'||(c)=='\n')

#ifndef  milli2nano
#define  milli2nano(x)          (1000000*(x))
#endif


static int  get_error_description(int error_code, char* desc, int max_length) ;
static int get_param_size(int type, int ind);

static int setLogConfig(const char* instanceName, int filterID, int logLevel, llm_on_log_event_t logger,
                 void* user, int update, int *ec);
static int unsetLogConfig(const char* instanceName, int filterID);

#endif

#endif
