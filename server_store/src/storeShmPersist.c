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

/*********************************************************************/
/*                                                                   */
/* Module Name: Persist.c                                            */
/*                                                                   */
/* Description: Shm disk persistance file                            */
/*                                                                   */
/*********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#define TRACE_COMP Store

#define USE_FLOCK 0

#define USE_STATS 0
#define USE_HISTO 0
#define FAST_WAIT_ROUNDS 100

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <dirent.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/vfs.h>
#include <stdbool.h>
#include <execinfo.h>
#if USE_FLOCK&1
#include <sys/file.h>
#elif USE_FLOCK&2
#include <unistd.h>
#include <fcntl.h>
#endif
#include "storeDiskUtils.h"
#include "storeUtils.h"
#include "storeMemory.h"
#include "storeMemoryHA.h"
#include "storeHighAvailability.h"
#include "storeShmPersist.h"

typedef enum
{
  PERSIST_STATE_COMPLETED=0,
  PERSIST_STATE_PENDING=1,
  PERSIST_STATE_WORKING=2,
  PERSIST_STATE_DONE=4
} persistStage_t ; 

typedef struct
{
  uint64_t   ts ; 
  uint32_t   cid ; 
  uint32_t   len ; 
  uint32_t   nst ; 
  uint16_t   hsz ;
  uint16_t   tsz ;
} wHead_t ; 

typedef struct
{
  uint64_t   ts ; 
  uint32_t   cid ; 
  uint32_t   len ; 
} wTail_t ; 

typedef struct
{
  uint32_t                 cycleId ; 
  uint16_t                 recLen ; 
  uint8_t                  startGen ; 
  uint8_t                  genTr ; 
  uint8_t                  startFile[2] ; 
  uint8_t                  cleanStop ; 
  uint8_t                  coldStart ; 
  uint8_t                  isStandby ; 
  uint8_t                  stopToken[8];
  uint8_t                  pad[4] ;
} persistState_t ; 

typedef struct
{
  size_t ST_size ; 
  size_t ST_HWM ; 
  size_t ST_off ; 
  int    ST_fd ; 
  int    CPM_fd ; 
  int    CPG_fd ; 
  char   ST_fn[8] ; 
  char   CPM_fn[8] ; 
  char   CPG_fn[8] ; 
} persistFiles_t ; 

typedef struct
{
  uint32_t                      nLoops;
  uint32_t                      nCBs ;
  ismStore_AsyncCBStatsMode_t   mode;
  double                        nextTime;
  double                        currTime;
} cbStats ; 

typedef struct
{
  pthread_mutex_t                 lock[1];
  pthread_cond_t                  cond[1];
  ismStore_commitCBinfo_t        *rCBs ;
  ismStore_commitCBinfo_t        *wCBs ;
  ismStore_commitCBinfo_t        *pCBs ;
  uint32_t                        sizeR; 
  uint32_t                        sizeW; 
  uint32_t                        sizeP; 
  uint32_t                        numWCBs ; 
  uint32_t                        numRCBs ; 
  uint32_t                        numPCBs ; 
  cbStats                         stats ; 
  ismStore_AsyncThreadCBStats_t   lastPeriodStats;
} rcbQueue_t ; 

typedef struct
{
  pthread_mutex_t                 lock[1];
  pthread_cond_t                  cond[1];
  int                             haWorkS ; 
  int                             sigHA ; 
  int                             myInds[64] ; 
  int                             numInds ; 
} haSendThread_t ; 

#define  NUM_RCB_QUEUES  32
#define  MAX_ASYNC_THREADs 64

static int RCB_LWM=(4*1024);
static int RCB_HWM=(64*1024);
  
static double deliveryDelay=0e0;
#define LOCK_WAIT_TO      5e0
#define LOCK_WAIT_RT      3e-1
#define LOCK_MAX_ATTEMPTS 30

typedef struct
{
  pthread_t                       tid[3+2*MAX_ASYNC_THREADs];
  pthread_mutex_t                 rwLocks[4+MAX_ASYNC_THREADs];
  pthread_mutex_t                 lock[1];
  pthread_cond_t                  cond[1];
  pthread_mutex_t                 sigLock[1];
  pthread_cond_t                  sigCond[1];
  pthread_mutex_t                 haLockR[1];
  pthread_cond_t                  haCondR[1];
  ismStoe_DirInfo                 di[1];
  persistFiles_t                  pFiles[2][2];
  persistState_t                  PState[2] ; 
  ismStore_DiskTaskParams_t       DiskTaskParams_CPM[1] ; 
  ismStore_DiskTaskParams_t       DiskTaskParams_CPG[1] ; 
  ismStore_PersistParameters_t    PersistParams[1] ; 
  size_t                          wSize;
  void                           *wBuff;
  rcbQueue_t                      rcbQ[MAX_ASYNC_THREADs];
  haSendThread_t                  sndT[MAX_ASYNC_THREADs];
  double                          lwTO ; 
  double                          lwRT ; 
  uint32_t                        lwAttempts ; 
  uint32_t                        lwRaiseEvent;
  uint32_t                        numTHtx ; 
  uint32_t                        numTHrx ; 
  uint32_t                        numWCBs ; 
  uint32_t                        numRCBs ; 
  uint32_t                        totWCBs ; 
  uint32_t                        totRCBs ; 
  uint32_t                        totSkip ; 
  uint32_t                        emptCBs ; 
  uint32_t                        rcbHWM  ; 
  uint32_t                        goDown ; 
  uint32_t                        nSTs  ; 
  uint32_t                        go2Work ; 
  uint32_t                        gotWork ; 
  uint32_t                        init; 
  uint32_t                        thUp; 
  uint32_t                        curI ;  
  uint32_t                        curJ ;  
   int                            genClosed ; 
  uint32_t                        stFull ; 
  uint32_t                        iState ; 
  uint32_t                        jState ; 
  uint32_t                        fileTranAllowed ; 
  int                             fileWriteState[2] ;
  int                             fileReadState[2] ;
  int                             writeGenMsg;
  int                             PState_fd ; 
  char                            PState_fn[8] ; 
  ismStore_memHAChannel_t       **pChs ; 
  int                             pChsSize ; 
  int                             recoveryDone; 
  int                             needCP; 
  int                             waitCP; 
  int                             useSigTh;
  int                             sigWork ;
  int                             useYield;
  int                             stopCB;
  int                             cbStopped;
  int                             enableHA;
  int                             indL;
  int                             indH;
  int                             numRW;
  int                             indRW;
  int                             add2len;
  int                             add2sid;
  int                             add2ptr;
  int                             storeLocked;
} persistInfo_t ; 

typedef struct
{
  uint32_t cI ; 
  uint32_t cJ ; 
  uint32_t cId; 
  uint32_t bId; 
  size_t   nIo ; 
} persistReplay_t ; 


#if USE_STATS
typedef struct
{
  double  nS;
  double  nA;
  double  nB;
  double  nW;
  double  nT;
} persistStats_t ; 
persistStats_t pStats[1];
#endif

#define MSG_HEADER_SIZE (1*LONG_SIZE + 4*INT_SIZE + 1*SHORT_SIZE + 1)
#define PERSIST_COMPLETED(x) (x->pPersist->State == PERSIST_STATE_COMPLETED && (x->pPersist->AckSqn >= x->pPersist->MsgSqn || x->hStream == ismStore_memGlobal.hInternalStream || !ismSTORE_HASSTANDBY))
#define INC_CID(x) {if(!(++(x))) (x)++;TRACE(5,"%s_%d: INC_CID incremented %s to %u\n",__FUNCTION__,__LINE__,#x,x);}

extern ismStore_memGlobal_t ismStore_memGlobal;
extern void ism_engine_threadInit(uint8_t isStoreCrit);
extern void ism_engine_threadTerm(uint8_t closeStoreStream);
//extern void ism_common_stack_trace(void);
static persistInfo_t pInfo[1] ; 

static int ism_storePersist_prepareCP(int cI, int cJ);
static int ism_storePersist_writeCP(void);
static int ism_store_persistIO(int fd, char *buff, size_t batch, int ioIn);
static int ism_store_persistParseST(ismStore_memHAChannel_t *pHAChannel);
static int ism_store_persistParseGenST(ismStore_memHAChannel_t *pHAChannel);
static int ism_store_persistReplayFile(persistReplay_t *pReplay);
static int ism_store_persistReplayST(char *pData);
static int ism_storePersist_calcStopToken(void *buff, size_t buffLen, uint8_t *token, int tokenLen);
static int ism_storePersist_completeST_(ismStore_memStream_t  *pStream);
static void ism_store_persistHandleGenTran(void);
static void ism_store_persistHandleFileFlip(void);
static int ism_store_persistWaitForSpace(ismStore_memStream_t  *pStream);
static int32_t ism_storePersist_zeroFile(int fd, size_t size);

#if USE_FLOCK
#define  FLOCK_SH 1
#define  FLOCK_EX 2
#define  FLOCK_UN 4
#define  FLOCK_NB 8

//  LOCK_SH , LOCK_EX , LOCK_UN , LOCK_NB
static inline int ism_store_flock(int fd, int flags)
{
  int rc ; 
 #if USE_FLOCK&2
  struct flock fl[1];
  int cmd = (flags&FLOCK_NB) ? F_SETLK : F_SETLKW ; 
  fl->l_type   = (flags&FLOCK_SH) ? F_RDLCK : ((flags&FLOCK_EX) ? F_WRLCK : F_UNLCK) ; 
  fl->l_whence = SEEK_SET ; 
  fl->l_start  = 0 ; 
  fl->l_len    = 1 ; 
  rc = fcntl(fd, cmd, fl) ; 
 #else
  int op = (flags&FLOCK_SH) ? LOCK_SH : ((flags&FLOCK_EX) ? LOCK_EX : LOCK_UN) ; 
  op |= (flags&FLOCK_NB)*LOCK_NB ; 
  rc = flock(fd, op) ; 
 #endif
  return rc ; 
}
#endif


#if USE_HISTO

#define sysTime() ism_common_readTSC()
#define   rmmHistEntry_t unsigned int

#define MAX_PCT_VALS 16
#define HIST_SIZE 1000
#define UNIT_SIZE 1e-3

typedef struct
{
   int                hist_size ;
  rmmHistEntry_t      base_num ;
  rmmHistEntry_t      tot_num ;
  rmmHistEntry_t      num ;
  rmmHistEntry_t      big ;
  rmmHistEntry_t     *hist ;
   double             base_time ;
   double             duration ;
   double             factor ;
   double             sum ;
   double             avr ;
   double             min ;
   double             max ;
   double             p_args[MAX_PCT_VALS] ;
   double             p_vals[MAX_PCT_VALS] ;
} StatInfo ;

#define CB_ARRAY_SIZE (1<<18)
typedef struct
{
  double     st[CB_ARRAY_SIZE];
  uint32_t   sq[CB_ARRAY_SIZE];
} sqstPairs ; 

static StatInfo *cbSi[6] ; 
/*
 0 -> CB_DELAY   10 sec / 1 mil (10000 1e-3)
 1 -> ACK_DELAY  10 sec / 1 mil (10000 1e-3)
 2 -> CB_CALL0   10 sec / 10 mil (1000 1e-2)
 3 -> CB_CALL1   10 mil / 10 mic (1000 1e-5)
 4 -> FILL_BUF    2 sec / 100 mic (20000 1e-4)
 5 -> SEND_BUF    2 sec / 100 mic (20000 1e-4)
*/

static StatInfo *hist_alloc(int hist_size, double unit_size, rmmHistEntry_t tot_num) ;
static void hist_free(StatInfo *si) ;
static void hist_reset(StatInfo *si, rmmHistEntry_t tot_num) ;
static void hist_calc(StatInfo *si, rmmHistEntry_t tot_num) ;
static void hist_add(StatInfo *si, double val) ;


void hist_add(StatInfo *si, double val)
{
  rmmHistEntry_t t ;
  if ( !si )
    return ;

  t = si->num++ ;
  if ( t > si->num )
  {
    hist_reset(si,si->tot_num) ;
    si->num = 1 ;
  }

  if ( si->hist_size > 0 )
  {
    int ind = lrint(val*si->factor) ;
    if ( ind >= 0 && ind < si->hist_size )
     si->hist[ind]++ ;
    else
    {
      si->big++ ;
    }
  }
//else
  {
    si->sum += val ;
    if ( si->min > val )
         si->min = val ;
    if ( si->max < val )
         si->max = val ;
  }
}

void hist_calc(StatInfo *si, rmmHistEntry_t tot_num)
{
  if ( !si )
    return ;

  si->tot_num = tot_num - si->base_num ;
  if ( si->tot_num < si->num )
       si->tot_num = si->num ;

  si->duration = sysTime() - si->base_time ;

  if ( si->num <= 0 )
    return ;

  if ( si->hist_size > 0 )
  {
    int i,j ;
    double num, val , inc ;
    double p_nums[MAX_PCT_VALS] ;
    memset(    p_nums,0,sizeof(    p_nums)) ;
    memset(si->p_vals,0,sizeof(si->p_vals)) ;

    num = (double)si->num ;
    for ( j=0 ; j<MAX_PCT_VALS ; j++ )
      p_nums[j] = num*si->p_args[j] ;

    num = 0 ; 
    inc = 1e0 / si->factor ;
    for ( i=0 , j=0 , val=inc/2 ; i<si->hist_size ; i++ , val += inc)
    {
      if ( si->hist[i] <= 0 )
        continue ;

      num += (double)si->hist[i] ;
      for ( ; j<MAX_PCT_VALS ; j++ )
      {
        if ( p_nums[j] > num )
          break ;
        si->p_vals[j] = val ;
      }
    }
  }
//else
    si->avr = si->sum / (double)si->num ;

}


void hist_reset(StatInfo *si, rmmHistEntry_t tot_num)
{
  if ( !si )
    return ;
  if ( si->hist_size > 0 && si->hist )
    memset(si->hist,0,si->hist_size*sizeof(rmmHistEntry_t)) ;

  si->base_num = tot_num ;
  si->tot_num  = 0 ; 
  si->num  = 0 ; 
  si->big  = 0 ; 
  si->base_time = sysTime() ;
  si->duration = 0 ; 
  si->sum = 0e0 ;
  si->avr = 0e0 ;
  si->min = 1e30 ;
  si->max = -si->min ;
}

StatInfo *hist_alloc(int hist_size, double unit_size, rmmHistEntry_t tot_num)
{
  StatInfo *si ;

  if ( (si=(StatInfo *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,1),sizeof(StatInfo))) == NULL )
    return NULL ;

  if ( hist_size > 0 )
  {
    if ( (si->hist=(unsigned int *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,2),hist_size*sizeof(rmmHistEntry_t))) == NULL )
    {
      ism_common_free(ism_memory_store_misc,si) ;
      return NULL ;
    }
    si->hist_size = hist_size ;
    if ( unit_size > 0e0 )
      si->factor = 1e0/unit_size ;
    else
      si->factor = 1e0 ;
  }

  hist_reset(si,tot_num) ;

  return si ;
}

void hist_free(StatInfo *si)
{
  if ( !si )
    return ;
  if ( si->hist )
    ism_common_free(ism_memory_store_misc,si->hist) ;
  ism_common_free(ism_memory_store_misc,si) ;
}
#endif  // USE_HISTO

static void ism_store_persistFatal(int state, int line)
{
  TRACE(1,"Disk persistence thread encountered an unrecoverabale error at %d, errno=%d (%s) ; Quitting disk persistence!!!\n",line,errno,strerror(errno));
  pInfo->goDown = 2 ; 
  ism_store_memSetStoreState(state,1) ; 
}

static void ism_store_persistPrintGenHeader(void *buff, int line)
{
  ismStore_memGenHeader_t *pGenHeader = (ismStore_memGenHeader_t *)buff ; 
  TRACE(5,"line=%d , GenHeader: StrucId=%x, GenId=%hu, State=%hu, PoolsCount=%hu, DescriptorStructSize=%u, SplitItemStructSize=%u, Version=%lu, MemSizeBytes=%lu\n",line,
          pGenHeader->StrucId,pGenHeader->GenId,pGenHeader->State,pGenHeader->PoolsCount,pGenHeader->DescriptorStructSize,pGenHeader->SplitItemStructSize,pGenHeader->Version,pGenHeader->MemSizeBytes);
}
static void ism_store_persistPrintMgmtHeader(void *buff, int line)
{
  ismStore_memMgmtHeader_t *pMgmtHeader = (ismStore_memMgmtHeader_t *)buff ; 
  ismStore_memGenHeader_t *pGenHeader = (ismStore_memGenHeader_t *)buff ; 
  TRACE(5,"line=%d ,MgmtHeader: StrucId=%x, GenId=%hu, State=%hu, PoolsCount=%hu, DescriptorStructSize=%u, SplitItemStructSize=%u, Version=%lu, MemSizeBytes=%lu, ActiveGenIndex=%u, ActiveGenId=%u, NextAvailableGenId=%u\n",line,
          pGenHeader->StrucId,pGenHeader->GenId,pGenHeader->State,pGenHeader->PoolsCount,pGenHeader->DescriptorStructSize,pGenHeader->SplitItemStructSize,pGenHeader->Version,pGenHeader->MemSizeBytes,pMgmtHeader->ActiveGenIndex,pMgmtHeader->ActiveGenId,pMgmtHeader-> NextAvailableGenId);
}

static void ism_store_persistPrintGidChunk(void *buff, int line)
{
   char str[4096];
   int j,k=0;
   ismStore_memDescriptor_t *pDescriptor = (ismStore_memDescriptor_t *)buff ; 
   ismStore_memGenIdChunk_t *pGenIdChunk = (ismStore_memGenIdChunk_t *)(pDescriptor + 1);
   k += snprintf(str+k,4096-k,"GenIdChunk: line=%u, type=%u, len=%u, count=%u :",line,pDescriptor->DataType,pDescriptor->DataLength,pGenIdChunk->GenIdCount);
   for ( j=0 ; j<pGenIdChunk->GenIdCount ; j++ )
      k += snprintf(str+k,4096-k," %u,",pGenIdChunk->GenIds[j]) ; 
   TRACE(5,"%s\n",str);
}

static int ism_store_persistWritePState(int line)
{
  char *f;
  if ( lseek(pInfo->PState_fd, 0, SEEK_SET) < 0 )
    f = "lseek" ; 
  else
  if ( write(pInfo->PState_fd, pInfo->PState, sizeof(persistState_t)) != sizeof(persistState_t) )
    f = "write" ; 
  else
  if ( fsync(pInfo->PState_fd) < 0 )
    f = "fsync" ; 
  else
  {
    TRACE(5,"%s: write PState: line=%u, cid=%u, curI=%u, curJ=%u, genTr=%u\n",__FUNCTION__,line,pInfo->PState[0].cycleId,pInfo->PState[0].startGen,pInfo->PState[0].startFile[0],pInfo->PState[0].genTr);
    return 0 ; 
  }
  TRACE(1,"%s: %s failed ; errno=%d (%s)\n",__FUNCTION__,f,errno,strerror(errno)) ; 
  ism_store_persistFatal(ismSTORE_STATE_DISKERROR , line) ; 
  return -1 ; 
}


int ism_storePersist_wait4lock(void)
{
  while (!pInfo->goDown && !pInfo->storeLocked )
  {
    pthread_mutex_lock(pInfo->lock) ; 
    pInfo->go2Work = 1;
    pthread_cond_signal(pInfo->cond);
    pthread_mutex_unlock(pInfo->lock) ; 
    su_sleep(1,SU_MIL);
  }
  return ISMRC_OK ; 
}

int ism_storePersist_flushQ(int wait4ack, int milli2wait)
{
  int i,m,j,k,l, ind=0;
  ismStore_memStream_t  *pStream ; 
  double et=0;
  if ( milli2wait > 0 )
    et = ism_common_readTSC() + 1e-3*milli2wait ; 
  while (!pInfo->goDown && (milli2wait <= 0 || ism_common_readTSC() < et) )
  {
    j = k = l = 0 ; 
    pthread_mutex_lock(&pInfo->rwLocks[0]);
    for (i=0, m=0 ; m<ismStore_memGlobal.StreamsCount ; i++)
    {
      if ((pStream = ismStore_memGlobal.pStreams[i]) != NULL)
      {
        m++ ; 
        ismSTORE_MUTEX_LOCK(&pStream->Mutex);
        j =  (pStream->pPersist->State & PERSIST_STATE_PENDING) ;
        if ( pStream->pPersist->Buf1Len )
        {
          ind = pStream->pPersist->indTx ; 
          k = 1 ; 
        } 
        if ( wait4ack )
          l =!PERSIST_COMPLETED(pStream) ; 
        ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);
        if ( j|k|l ) break ; 
      }
    }
    pthread_mutex_unlock(&pInfo->rwLocks[0]);
    if (!(j|k|l) )
      return ISMRC_OK ; 
    if ( j )
    {
      pthread_mutex_lock(pInfo->lock) ; 
      pInfo->go2Work = 1;
      pthread_cond_signal(pInfo->cond);
      pthread_mutex_unlock(pInfo->lock) ; 
    }
    if ( k )
    {
      haSendThread_t *sndT = &pInfo->sndT[ind] ; 
      pthread_mutex_lock(sndT->lock);
      sndT->haWorkS = 1 ; 
      pthread_cond_signal(sndT->cond) ; 
      pthread_mutex_unlock(sndT->lock);
    }
    su_sleep(1,SU_MIC) ; 
  }
  return ISMRC_OK ; 
}

double ism_storePersist_getTimeStamp(void)
{
  int i,m;
  ismStore_memStream_t  *pStream ; 
  double ct = ism_common_readTSC(), dt;

  dt = ct ; 
  pthread_mutex_lock(&pInfo->rwLocks[0]);
  for (i=0, m=0 ; m<ismStore_memGlobal.StreamsCount ; i++)
  {
    if ((pStream = ismStore_memGlobal.pStreams[i]) != NULL)
    {
      m++ ; 
      if ( i == ismStore_memGlobal.hInternalStream ) continue ; 
      if ( pStream->pPersist->curCB->TimeStamp > pStream->pPersist->TimeStamp &&
           ct > pStream->pPersist->TimeStamp )
           ct = pStream->pPersist->TimeStamp ;
    }
  }
  pthread_mutex_unlock(&pInfo->rwLocks[0]);
  TRACE(9,"%s: ct= %g, dt= %g\n",__FUNCTION__,ct,dt);
  return ct ; 
}

int ism_storePersist_wrLock(void)
{
  int i;
  for ( i=0 ; i<pInfo->numRW ; i++ )
    pthread_mutex_lock(&pInfo->rwLocks[i]) ;
  return ISMRC_OK ; 
}

int ism_storePersist_wrUnlock(void)
{
  int i;
  for ( i=pInfo->numRW-1 ; i>=0 ; i-- )
    pthread_mutex_unlock(&pInfo->rwLocks[i]) ;
  return ISMRC_OK;
}

static void *ism_store_persistSigThread(void *arg, void * context, int value)
{
  ismStore_memStream_t  *pStream ; 
  pthread_mutex_t *rdLock;
  int i,m;

  pthread_mutex_lock(pInfo->lock) ; 
  pInfo->thUp++ ;
  rdLock = &pInfo->rwLocks[pInfo->indRW++] ; 
  pthread_mutex_unlock(pInfo->lock) ; 
  TRACE(5, "The %s thread is started\n", __FUNCTION__);

  for(;;)
  {
    ism_common_backHome();
    pthread_mutex_lock(pInfo->sigLock);
    while (!pInfo->goDown && !pInfo->sigWork )
      pthread_cond_wait(pInfo->sigCond, pInfo->sigLock);
    pInfo->sigWork = 0 ; 
    pthread_mutex_unlock(pInfo->sigLock);
    if ( pInfo->goDown )
      break ; 
    ism_common_going2work();
    pthread_mutex_lock(rdLock);
    for (i=0, m=0 ; m<ismStore_memGlobal.StreamsCount ; i++)
    {
      if ((pStream = ismStore_memGlobal.pStreams[i]) != NULL)
      {
        m++ ; 
        ismSTORE_MUTEX_LOCK(&pStream->Mutex);
        if ( (pStream->pPersist->State & PERSIST_STATE_DONE) )
        {
          pStream->pPersist->State &= ~PERSIST_STATE_DONE ;
          if ( pStream->pPersist->Waiting ) ismSTORE_COND_BROADCAST(&pStream->Cond);
        }
        ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);
      }
    }
    pthread_mutex_unlock(rdLock);
  }

  pthread_mutex_lock(pInfo->lock) ; 
  pInfo->thUp-- ;
  pthread_mutex_unlock(pInfo->lock) ; 
  TRACE(5, "The %s thread has stopped\n", __FUNCTION__);

  return NULL;
}

static void *ism_store_persistHaSThread(void *arg, void * context, int value)
{
  ismStore_memStream_t  *pStream ; 
  int i,m,rc, doClean=0, myId , numUpd=0 ; 
  ismStore_persistInfo_t *pPersist ; 
  ismStore_memHAChannel_t *pHAChannel ; 
  haSendThread_t *sndT;
  pthread_mutex_t *rdLock ; 

  pthread_mutex_lock(pInfo->lock) ; 
  pInfo->thUp++ ;
  myId = value ;
  rdLock = &pInfo->rwLocks[pInfo->indRW++] ; 
  pthread_mutex_unlock(pInfo->lock) ; 
  TRACE(5, "The %s thread is started (%d)\n", __FUNCTION__,myId);
  sndT = &pInfo->sndT[myId] ; 

  for (;;)
  {
    ism_common_backHome();
    pthread_mutex_lock(sndT->lock) ; 
    while (!pInfo->goDown && !sndT->haWorkS )
      pthread_cond_wait(sndT->cond, sndT->lock);
    sndT->haWorkS = 0 ; 
    pthread_mutex_unlock(sndT->lock);
    if ( pInfo->goDown )
      break ; 
    pthread_mutex_lock(rdLock);
    ism_common_going2work();
    if ( numUpd != ismStore_memGlobal.StreamsUpdCount )
    {
      sndT->numInds = 0 ; 
      for (i=0, m=0 ; m<ismStore_memGlobal.StreamsCount ; i++)
      {
        if ((pStream = ismStore_memGlobal.pStreams[i]) != NULL)
        {
          m++ ; 
          if ( i == ismStore_memGlobal.hInternalStream ) continue ; 
          if ( pStream->pPersist->indTx != myId ) continue ;  
          sndT->myInds[sndT->numInds++] = i ; 
        }
      }
      numUpd = ismStore_memGlobal.StreamsUpdCount ;
    }
    if ( ismSTORE_HASSTANDBY )
    {
      doClean = 1 ; 
      for ( m=0 ; m<sndT->numInds ; m++)
      {
        i = sndT->myInds[m] ; 
        if ((pStream = ismStore_memGlobal.pStreams[i]) != NULL)
        {
          ismSTORE_MUTEX_LOCK(&pStream->Mutex);
          pPersist = pStream->pPersist ; 
          pHAChannel = pStream->pHAChannel ; 
          if ( pPersist->Buf1Len )
          {
            char *buf ; 
            uint32_t len ;
            if ( (rc = ism_storeHA_allocateBuffer(pHAChannel->hChannel, &buf, &len)) == StoreRC_OK)
            {
              if ( len >= pPersist->Buf1Len )
              {
                memcpy(buf, pPersist->Buf1, pPersist->Buf1Len) ; 
                len = pPersist->Buf1Len ; 
                pPersist->Buf1Len = 0 ; 
                ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);
               #if USE_HISTO
                double ct = ism_common_readTSC() ; 
                if ( pPersist->ct )
                {
                  hist_add(cbSi[4], ct-pPersist->ct) ; 
                  pPersist->ct = 0e0 ; 
                }
               #endif
                if ((rc = ism_storeHA_sendMessage(pHAChannel->hChannel, buf, len)) != StoreRC_OK)
                {
                   int lvl = (rc == StoreRC_HA_ConnectionBroke) ? 9 : 1 ;
                   TRACE(lvl, "Failed to send an HA message (ChannelId %d, BufLen %u). error code %d\n",
                            pHAChannel->ChannelId, len, rc);
                }
               #if USE_HISTO
                hist_add(cbSi[5], ism_common_readTSC()-ct) ; 
               #endif
                pPersist->numPackets++ ; 
                pPersist->numBytes += len ; 
                TRACE(9,"persistHA: ism_storeHA_sendMessage(pHAChannel->hChannel, buf, len) %p %u\n",buf,len);
              }
              else
              {
                TRACE(1,"HA buffer is too small (%u < %u).  This is an internal error!\n",len, pPersist->Buf1Len);
                pPersist->Buf1Len = 0 ; 
                ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);
                su_sleep(16,SU_MIL);
              }
            }
            else
            {
              int lvl = (rc == StoreRC_HA_ConnectionBroke) ? 9 : 1 ;
              TRACE(lvl, "Failed to allocate a buffer for an HA message (ChannelId %d, BufLen %u). error code %d\n",
                       pHAChannel->ChannelId, pPersist->Buf1Len, rc);
              pPersist->Buf1Len = 0 ; 
              ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);
            }
          }
          else
            ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);
        }
      }
    }
    else
    if ( doClean )
    {
      doClean = 0 ; 
      for ( m=0 ; m<sndT->numInds ; m++)
      {
        i = sndT->myInds[m] ; 
        if ((pStream = ismStore_memGlobal.pStreams[i]) != NULL)
        {
          ismSTORE_MUTEX_LOCK(&pStream->Mutex);
          pPersist = pStream->pPersist ; 
          if ( pPersist->Buf1Len )
            pPersist->Buf1Len = 0 ; 
          ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);
        }
      }
    }
    pthread_mutex_unlock(rdLock);
  }

  pthread_mutex_lock(pInfo->lock) ; 
  pInfo->thUp-- ;
  pthread_mutex_unlock(pInfo->lock) ; 
  TRACE(5, "The %s thread has stopped\n", __FUNCTION__);

  return NULL;
}

static void *ism_store_persistHaRThread(void *arg, void * context, int value)
{
  ismStore_memStream_t  *pStream ; 
  int i,m,rc, doWait=1, doClean=0;
  int noRead, numNoRead=0 ; 
  ismStore_memHAAck_t ack;
  ismStore_persistInfo_t *pPersist ; 
  ismStore_memHAChannel_t *pHAChannel ; 
  pthread_mutex_t *rdLock;

  pthread_mutex_lock(pInfo->lock) ; 
  pInfo->thUp++ ;
  rdLock = &pInfo->rwLocks[pInfo->indRW++] ; 
  pthread_mutex_unlock(pInfo->lock) ; 
  TRACE(5, "The %s thread is started\n", __FUNCTION__);

  for (;;)
  {
    ism_common_backHome();
    if ( doWait )
    {
      rc = -1 ; 
      if ( ismSTORE_HASSTANDBY )
        rc = ism_storeHA_pollOnAllChanns(1);
      if ( rc < 0 )
        su_sleep(1,SU_MIL);
      if ( pInfo->goDown )
        break ; 
      if ( rc == 0 ) // We go to read only when there are events on any of the sockets
        continue ; 
    }
    else
      doWait = 1 ; 
    ism_common_going2work();
    pthread_mutex_lock(rdLock);
    if ( ismSTORE_HASSTANDBY )
    {
      doClean = 1 ; 
      noRead = 1 ; 
      for (i=0, m=0 ; m<ismStore_memGlobal.StreamsCount ; i++)
      {
        if ((pStream = ismStore_memGlobal.pStreams[i]) != NULL)
        {
          m++ ; 
          if ( i == ismStore_memGlobal.hInternalStream ) continue ; 
          ismSTORE_MUTEX_LOCK(&pStream->Mutex);
          pPersist = pStream->pPersist ; 
          pHAChannel = pStream->pHAChannel ; 
          if ( pPersist->AckSqn < pPersist->MsgSqn )
          {
            noRead = 0 ; 
            pHAChannel->MsgSqn = pPersist->MsgSqn ; 
            memset(&ack, '\0', sizeof(ack));
            if ( (rc = ism_store_memHAReceiveAck(pHAChannel, &ack, 1)) == StoreRC_OK )
            {
              int ind = pStream->pPersist->indRx ; 
              rcbQueue_t *rcbQ = &pInfo->rcbQ[ind] ; 
             #if USE_HISTO
              uint64_t AckSqn = pPersist->AckSqn ; 
              double ct = ism_common_readTSC() ; 
              double *cts = (double *)pPersist->RSRV ; 
             #endif
              pPersist->AckSqn = ack.AckSqn+1 ; 
              if ( pStream->pPersist->Waiting ) ismSTORE_COND_BROADCAST(&pStream->Cond);
              pthread_cond_signal(rcbQ->cond) ; 
             #if USE_HISTO
              if ( cts )
              {
                while (  AckSqn < pPersist->AckSqn )
                {
                  ind = AckSqn & (CB_ARRAY_SIZE-1) ;
                  if ( cts[ind] )
                  {
                    hist_add(cbSi[1], ct-cts[ind]) ; 
                    cts[ind] = 0e0 ; 
                  }
                  AckSqn++ ; 
                }
              }
             #endif
             #if USE_STATS
              pStats->nA++ ; 
             #endif
              doWait = 0 ;  // this is a MUST
            }
            else
            if ( rc != StoreRC_HA_WouldBlock )
            {
              pPersist->AckSqn = pPersist->MsgSqn ; 
              if ( pStream->pPersist->Waiting ) ismSTORE_COND_BROADCAST(&pStream->Cond);
            }
          }
          ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);
        }
      }
      if ( noRead )
      {
        numNoRead++ ;
        if ( (numNoRead % 300000) == 1 )
        {
          TRACE(3, "Poll returns socket events, but no Ack is expected on any of the channels.  This has happend %d times.\n", numNoRead);
        }     
        su_sleep(1,SU_MIL); 
      }
    }
    else
    if ( doClean )
    {
      doClean = 0 ; 
      for (i=0, m=0 ; m<ismStore_memGlobal.StreamsCount ; i++)
      {
        if ((pStream = ismStore_memGlobal.pStreams[i]) != NULL)
        {
          m++ ; 
          if ( i == ismStore_memGlobal.hInternalStream ) continue ; 
          ismSTORE_MUTEX_LOCK(&pStream->Mutex);
          pPersist = pStream->pPersist ; 
          if ( pPersist->AckSqn < pPersist->MsgSqn )
          {
            pPersist->AckSqn = pPersist->MsgSqn ; 
            if ( pStream->pPersist->Waiting ) ismSTORE_COND_BROADCAST(&pStream->Cond);
          }
          ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);
        }
      }
    }
    pthread_mutex_unlock(rdLock);
  }

  pthread_mutex_lock(pInfo->lock) ; 
  pInfo->thUp-- ;
  pthread_mutex_unlock(pInfo->lock) ; 
  TRACE(5, "The %s thread has stopped\n", __FUNCTION__);

  return NULL;
}

int ism_storePersist_stopCB(void)
{
  int i, tout=0, rc=ISMRC_OK ; 
  rcbQueue_t *rcbQ ; 
  if (pInfo->init < PERSIST_STATE_STARTED )
  {
    TRACE(5, "%s: pInfo->init = %d, return OK\n", __FUNCTION__,pInfo->init);
    return ISMRC_OK ; 
  }

  for ( i=0 ; i<pInfo->numTHrx ; i++ )
  {
    rcbQ = &pInfo->rcbQ[i] ; 
    pthread_mutex_lock(rcbQ->lock);
    pInfo->stopCB = 1;
    pthread_cond_signal(rcbQ->cond) ; 
    pthread_mutex_unlock(rcbQ->lock);
  }
  TRACE(5, "%s: waiting for %d async callback threads to stop deliver callbacks\n", __FUNCTION__,pInfo->numTHrx);
  pthread_mutex_lock(pInfo->lock) ; 
  while ( pInfo->cbStopped < pInfo->numTHrx )
  {
    if (tout++ > 2000) // approx 2 sec
    {
      rc = ISMRC_StoreBusy;
      break;
    }
    pthread_mutex_unlock(pInfo->lock) ; 
    su_sleep(1,SU_MIL) ; 
    pthread_mutex_lock(pInfo->lock) ; 
  }
  pthread_mutex_unlock(pInfo->lock) ; 
  if (rc == ISMRC_OK)
  {
    TRACE(5, "%s: all %d async callback threads stopped delivering callbacks\n", __FUNCTION__,pInfo->numTHrx);
  }
  else
  {
    TRACE(3, "%s: not all %d async callback threads stopped, number stopped %d\n", __FUNCTION__,pInfo->numTHrx,pInfo->cbStopped);
  }

  return rc ; 
}

static void *ism_store_persistAsyncThread(void *arg, void * context, int value)
{
  int j=1,nCBs, myId , off=0, cbStopped=0, cbPaused ; 
  rcbQueue_t *rcbQ ; 
  struct timespec reltime={0,100000};
  cbStats *stats ; 

 #if USE_STATS
  persistStats_t qStats[1];
  double ct;
  #if USE_HISTO
   double p_args[] = {.01e0, .05e0, .10e0, .25e0, .50e0, .75e0, .90e0, .95e0, .99e0};
  #endif
 #endif

  pthread_mutex_lock(pInfo->lock) ; 
  myId = value ;  
  pInfo->thUp++ ; 
  pthread_mutex_unlock(pInfo->lock) ; 
  TRACE(5, "The %s thread (id %d) is started\n", __FUNCTION__,myId);
  rcbQ = &pInfo->rcbQ[myId] ; 
  stats = &rcbQ->stats ; 

    // wait for the Store to be ACTIVE to allow a stream to be created when ism_engine_threadInit is called 
  while (!pInfo->goDown && !pInfo->stopCB && ismStore_memGlobal.State != ismSTORE_STATE_ACTIVE)
  {
    su_sleep(1,SU_MIL) ;
  }
  if (!pInfo->goDown && !pInfo->stopCB)
  {
    ism_engine_threadInit(pInfo->numTHrx+myId);
    TRACE(5, "The %s thread (id %d) is running\n", __FUNCTION__,myId);
  }
  else
  {
    TRACE(5, "The %s thread (id %d) was stopped (%d %d) before the store became active\n", __FUNCTION__,myId,pInfo->goDown,pInfo->stopCB);
  }

  pthread_mutex_lock(rcbQ->lock);
 #if USE_STATS
//if (!myId)  
    qStats->nT = ism_common_readTSC()+1e1; 
 #endif
  memset(stats,0,sizeof(cbStats));
  stats->mode = pInfo->PersistParams->AsyncCBStatsMode;
  stats->nextTime = ism_common_readTSC();

  while (!(pInfo->goDown))
  {
    stats->nLoops++ ; 
    ism_common_going2work();
    if (pInfo->stopCB && !cbStopped)
    {
      pthread_mutex_unlock(rcbQ->lock);
      pthread_mutex_lock(pInfo->lock) ;
      if (pInfo->stopCB && !cbStopped)
      {
        cbStopped = 1;
        pInfo->cbStopped++ ; 
      }
      pthread_mutex_unlock(pInfo->lock) ;
      pthread_mutex_lock(rcbQ->lock);
    }
    if ( rcbQ->numRCBs + rcbQ->numPCBs )
    {
      int sigHA=0 ; 
     #if USE_STATS
      ct = ism_common_readTSC() ; 
     #endif
      if ( !rcbQ->numPCBs )
      {
        uint32_t v = rcbQ->sizeR ; 
        ismStore_commitCBinfo_t *p = rcbQ->rCBs ; 
        rcbQ->rCBs = rcbQ->pCBs ; 
        rcbQ->pCBs = p ; 
        rcbQ->sizeR = rcbQ->sizeP ; 
        rcbQ->sizeP = v ; 
        rcbQ->numPCBs = rcbQ->numRCBs ; 
        rcbQ->numRCBs = 0 ; 
        off = 0 ; 
      }
      for ( j=0 ; j<rcbQ->numPCBs ; j++ )
      {
        ismStore_memStream_t  *pStream = rcbQ->pCBs[off+j].pStream ; 
        if ( pStream && pStream->pPersist && pStream->hStream != H_STREAM_CLOSED )
        {
          if ( XleY(pStream->pPersist->curCB->numCBs,rcbQ->pCBs[off+j].numCBs) )
            break ; 
          if ( deliveryDelay )
          {
            if ( rcbQ->pCBs[off+j].deliveryTime > ism_common_readTSC() )
              break ; 
          }
          else
          if ( ismSTORE_HASSTANDBY && pStream->pPersist->AckSqn <= rcbQ->pCBs[off+j].MsgSqn )
          {
            sigHA++ ; 
            break ; 
          }
        }
        pStream->pPersist->TimeStamp = rcbQ->pCBs[off+j].TimeStamp ; 
        TRACE(9,"%s: pPersist->TimeStamp = %g for stream %u\n",__FUNCTION__,pStream->pPersist->TimeStamp,pStream->hStream);
      }
     #if USE_STATS
      ct = ism_common_readTSC()-ct ; 
     #endif
     #if USE_STATS
      qStats->nB += ct ; 
     #endif
      if (!j )
      {
       #if USE_STATS
        ct = ism_common_readTSC() ; 
       #endif
        if ( sigHA )
        {
          ism_common_cond_timedwait(rcbQ->cond, rcbQ->lock, &reltime, 1);
        }
        else
        {
          pthread_mutex_unlock(rcbQ->lock);
          sched_yield();
          pthread_mutex_lock(rcbQ->lock);
        }
       #if USE_STATS
        ct = ism_common_readTSC()-ct ; 
        qStats->nA += ct ; 
       #endif
        if ( stats->mode )
        {
          if ( (stats->currTime=ism_common_readTSC()) > stats->nextTime ) 
          {
            rcbQ->lastPeriodStats.statsTime    = stats->currTime;
            rcbQ->lastPeriodStats.periodLength = 5e0+stats->currTime-stats->nextTime;
            rcbQ->lastPeriodStats.numCallbacks = stats->nCBs;
            rcbQ->lastPeriodStats.numLoops     = stats->nLoops;

            TRACE(stats->mode == ISM_STORE_ASYNCCBSTATSMODE_FULL ? 5: 7,
                  "asyncThread stats: thread %u executed %u loops and %u CBs in the last %f secs. Total pending CBs %u\n",
                  myId,stats->nLoops,stats->nCBs,rcbQ->lastPeriodStats.periodLength,rcbQ->numRCBs+rcbQ->numPCBs);
            stats->nLoops = stats->nCBs = 0 ; 
            stats->nextTime = stats->currTime + 5e0 ; 
          }
        }
        continue ; 
      }
     #if USE_STATS
      ct = ism_common_readTSC() ; 
     #endif
      nCBs = j ; 
      rcbQ->numPCBs -= nCBs ; 
      __sync_add_and_fetch(&pInfo->numRCBs,-nCBs) ; 
      pthread_mutex_unlock(rcbQ->lock);
      nCBs += off ; 
      if (!cbStopped)
      for ( j=off ; j<nCBs ; j++ )
      {
       #if USE_STATS && USE_HISTO
        double dt = ism_common_readTSC() ;
        hist_add(cbSi[0], (dt - rcbQ->pCBs[j].deliveryTime + deliveryDelay));
       #endif
        cbPaused = 0;
        while ((pInfo->iState == 1 || pInfo->jState == 1) && !pInfo->goDown )
        {
          if (!cbPaused )
          {
            cbPaused = 1 ; 
            TRACE(5, "The %s thread (id %d) is pausing delivering asyncCBs until the store is locked...\n", __FUNCTION__,myId);
          }
          pthread_mutex_lock(rcbQ->lock);
          ism_common_cond_timedwait(rcbQ->cond, rcbQ->lock, &reltime, 1);
          pthread_mutex_unlock(rcbQ->lock);
        }
        if ( cbPaused )
        {
          cbPaused = 0 ; 
          TRACE(5, "The %s thread (id %d) continues after pausing delivering asyncCBs until the store is locked...\n", __FUNCTION__,myId);
        }
        if ( !(j&15) ) ism_common_going2work();
        rcbQ->pCBs[j].pCallback(ISMRC_OK,rcbQ->pCBs[j].pContext) ; 
        if ( stats->mode )
        {
          stats->nCBs++ ; 
          if ( (stats->currTime=ism_common_readTSC()) > stats->nextTime ) 
          {
            rcbQ->lastPeriodStats.statsTime    = stats->currTime;
            rcbQ->lastPeriodStats.periodLength = 5e0+stats->currTime-stats->nextTime;
            rcbQ->lastPeriodStats.numCallbacks = stats->nCBs;
            rcbQ->lastPeriodStats.numLoops     = stats->nLoops;

            TRACE(stats->mode == ISM_STORE_ASYNCCBSTATSMODE_FULL ? 5: 7,
                  "asyncThread stats: thread %u executed %u loops and %u CBs in the last %f secs. Total pending CBs %u\n",
                  myId,stats->nLoops,stats->nCBs,rcbQ->lastPeriodStats.periodLength,rcbQ->numRCBs+rcbQ->numPCBs);
            stats->nLoops = stats->nCBs = 0 ; 
            stats->nextTime = stats->currTime + 5e0 ; 
          }
        }
       #if USE_STATS && USE_HISTO
        dt = ism_common_readTSC()-dt ;
        hist_add(cbSi[2], dt);
        hist_add(cbSi[3], dt);
       #endif
      }
     #if USE_STATS
      ct = ism_common_readTSC()-ct ; 
     #endif
      pthread_mutex_lock(rcbQ->lock);
     #if USE_STATS
      qStats->nB += ct ; 
     #endif
     #if USE_STATS
      qStats->nS += nCBs-off ;
     #endif
      off = j ; 
    }
    else
    {
      j = 1 ; 
     #if USE_STATS
      ct = ism_common_readTSC() ; 
     #endif
    //pthread_cond_wait(rcbQ->cond, rcbQ->lock);
      ism_common_cond_timedwait(rcbQ->cond, rcbQ->lock, &reltime, 1);
     #if USE_STATS
      ct = ism_common_readTSC()-ct ; 
      qStats->nW += ct ; 
     #endif
      if ( stats->mode )
      {
        if ( (stats->currTime=ism_common_readTSC()) > stats->nextTime ) 
        {
            rcbQ->lastPeriodStats.statsTime    = stats->currTime;
            rcbQ->lastPeriodStats.periodLength = 5e0+stats->currTime-stats->nextTime;
            rcbQ->lastPeriodStats.numCallbacks = stats->nCBs;
            rcbQ->lastPeriodStats.numLoops     = stats->nLoops;

          TRACE(stats->mode == ISM_STORE_ASYNCCBSTATSMODE_FULL ? 5: 7,
                "asyncThread stats: thread %u executed %u loops and %u CBs in the last %f secs. Total pending CBs %u\n",
                myId,stats->nLoops,stats->nCBs,rcbQ->lastPeriodStats.periodLength,rcbQ->numRCBs+rcbQ->numPCBs);
          stats->nLoops = stats->nCBs = 0 ; 
          stats->nextTime = stats->currTime + 5e0 ; 
        }
      }
    }
    if ( pInfo->rcbHWM && pInfo->numRCBs < RCB_LWM )
    {
      int i;
      pthread_mutex_unlock(rcbQ->lock);
      pthread_mutex_lock(pInfo->lock) ; 
      if ( pInfo->rcbHWM && pInfo->numRCBs < RCB_LWM && !pInfo->stopCB )
      {
        ismStore_memJob_t job;
        pInfo->rcbHWM = 0 ; 
        memset(&job, '\0', sizeof(job));
        job.JobType = StoreJob_UserEvent;
        job.Event.EventType = ISM_STORE_EVENT_CBQ_ALERT_OFF ; 
        ism_store_memAddJob(&job);
        TRACE(5,"Raising event ISM_STORE_EVENT_CBQ_ALERT_OFF;  numRCBs %u , lwm/hwm %u %u\n",pInfo->numRCBs,RCB_LWM,RCB_HWM);
        pthread_mutex_unlock(pInfo->lock) ; 
        for ( i=0 ; i<pInfo->numTHrx ; i++ )
        {
          pthread_mutex_lock(pInfo->rcbQ[i].lock);
          memset(&pInfo->rcbQ[i].stats,0,sizeof(cbStats));
          pInfo->rcbQ[i].stats.mode = pInfo->PersistParams->AsyncCBStatsMode;
          pthread_mutex_unlock(pInfo->rcbQ[i].lock);
        }
      }
      else
      pthread_mutex_unlock(pInfo->lock) ; 
      pthread_mutex_lock(rcbQ->lock);
    }
   #if USE_STATS
 // if (!myId)  
    {
      ct = ism_common_readTSC() ; 
      if ( qStats->nT < ct )
      {
        double dt = (ct - qStats->nT + 1e1) ; //*pInfo->PersistParams->PersistAsyncThreads;
        double cb = qStats->nS ; //*pInfo->PersistParams->PersistAsyncThreads;
        TRACE(1,"_dbg_S stats: rate: %f CBPS, Waiting: %f , Working: %f , Polling: %f\n",cb/dt,qStats->nW/dt,qStats->nB/dt,qStats->nA/dt);
        memset(qStats,0,sizeof(qStats));
        qStats->nT = ct + 1e1;
       #if USE_HISTO
        if (!myId)  
        {
          char *title[] = {"CB_DELAY ","ACK_DELAY","CB_CALL0 ","CB_CALL1 ","FILL_BUFF","SEND_BUF "};
          for ( int k=0 ; k<6 ; k++ )
          {
            StatInfo *si = cbSi[k] ; 
            memcpy(si->p_args, p_args, sizeof(p_args));
            hist_calc(si, pInfo->totWCBs) ; 
            TRACE(1,"_dbg_S stats: %s: min %g, p01 %g, p05 %g, p10 %g, p25 %g, p50 %g, avr %g, p75 %g, p90 %g, p95 %g, p99 %g, max %g, nbig %u, num %u\n",title[k],
                    si->min,si->p_vals[0],si->p_vals[1],si->p_vals[2],si->p_vals[3],si->p_vals[4],si->avr,
                    si->p_vals[5],si->p_vals[6],si->p_vals[7],si->p_vals[8],si->max,si->big,si->num);
            hist_reset(si, pInfo->totWCBs) ; 
          }
        }
       #endif
      }
    }
   #endif
  }
  pthread_mutex_unlock(rcbQ->lock);
  ism_common_backHome();
  ism_engine_threadTerm(0);  // Don't want to close the store stream

  pthread_mutex_lock(pInfo->lock) ; 
  pInfo->thUp-- ;
  pthread_mutex_unlock(pInfo->lock) ; 
  TRACE(5, "The %s thread  (id %d) has stopped\n", __FUNCTION__,myId);

  return NULL;
}

static void *ism_store_persistThread(void *arg, void * context, int value)
{
  int i,j,l,m,n, fd, rc=ISMRC_OK, pfd=-1, sigSig, sigHA, last=0 ;
  ismStore_memStream_t  *pStream ; 
  ismStore_persistInfo_t *pPersist ; 
  size_t batch, batch0 ; 
  int psm1, mask ;
  wHead_t *wHead, *wTail ; 
  void *hBuff, *tBuff;
  struct timespec reltime={0,100000};
  persistFiles_t *pF ; 
  double dummyIOtime=0e0;
  pthread_mutex_t *rdLock;
 #if USE_STATS
  double ct, t0, tG, tI, tJ, tW, tO, tT, tM, tS, nT, dt=0;
  int ss=0;
  struct counts
  {
  uint32_t                         numFullCB;
  uint32_t                         numFullBuff;
  uint32_t                         numCommit; 
  uint32_t                         numComplete;
  uint32_t                         numPackets;
  uint32_t                         numBytes;
  } NN ; 
 #endif

  i = ism_common_getIntConfig("Store.dummyIOtime", 0);
  if ( i )
  {
    dummyIOtime = 1e-6 * i ; 
  }
  //printf("dummyIOtime= %f\n",dummyIOtime);
  psm1 = pInfo->di->block ; 
  hBuff = pInfo->wBuff ; 
  tBuff = hBuff + psm1 ; 
  wHead = (wHead_t *)hBuff ; 
  wHead->hsz = (uint16_t)sizeof(wHead_t) ; 
  wHead->tsz = (uint16_t)sizeof(wTail_t) ; 
  psm1--; // it is assumed that block size is a power of 2
  mask = ~psm1 ; 
  batch0 = pInfo->wSize - sizeof(wHead_t) - sizeof(wTail_t) ; 

  pthread_mutex_lock(pInfo->lock) ; 
  pInfo->thUp++ ;
  rdLock = &pInfo->rwLocks[pInfo->indRW++];
  pthread_mutex_unlock(pInfo->lock) ; 
  TRACE(5, "The %s thread is started\n", __FUNCTION__);
  pInfo->fileTranAllowed = 1 ; 
 #if USE_STATS
  t0=tG=tI=tJ=tW=tO=tT=tM=tS=0e0 ; 
  nT = ism_common_readTSC() + 1e1 ; 
 #endif
  while (!pInfo->goDown )
  {
    ism_common_backHome();
    pthread_mutex_lock(pInfo->lock) ; 
    if ( pInfo->genClosed )
    {
      if ( !pInfo->iState && !pInfo->jState && pInfo->lwRT < ism_common_readTSC() )
      {
        pInfo->iState = 1 ; 
        TRACE(5,"%s: genTranPhase setting iState to %u\n",__FUNCTION__,pInfo->iState);
      }
    }
    else
    if ( pInfo->stFull && !pInfo->iState && !pInfo->jState && pInfo->lwRT < ism_common_readTSC() )
    {
      pInfo->stFull = 0 ; 
      pInfo->jState = 1 ; 
      TRACE(5,"%s: stFullPhase setting jState to %u\n",__FUNCTION__,pInfo->jState);
    }
    if ( pInfo->iState )
    {
     #if USE_STATS
      ct = ism_common_readTSC() ; 
      ism_store_persistHandleGenTran() ; 
      tI += ism_common_readTSC() - ct ; 
     #else
      ism_store_persistHandleGenTran() ; 
     #endif
    }
    else
    if ( pInfo->jState )
    {
     #if USE_STATS
      ct = ism_common_readTSC() ; 
      ism_store_persistHandleFileFlip() ; 
      tJ += ism_common_readTSC() - ct ; 
     #else
      ism_store_persistHandleFileFlip() ; 
     #endif
    }
    pthread_mutex_unlock(pInfo->lock) ; 
    if ( pInfo->goDown )
      continue ; 

    batch = batch0 ; 
   #if USE_STATS
    ct = ism_common_readTSC() ; 
    if ( ct > nT )
    {
      ss = 1 ; 
      dt = ct - nT + 1e1 ; 
      nT = ct + 1e1 ; 
    }
    else
      ss = 0 ; 
   #endif
    sigSig = 0 ; 
    sigHA = 0 ; 
    tBuff = hBuff + sizeof(wHead_t) ; 
    if ( pInfo->numWCBs ) 
    {
      rcbQueue_t *rcbQ ; 
      for ( i=0 ; i<pInfo->numTHrx ; i++ )
      {
        rcbQ = &pInfo->rcbQ[i] ; 
        if (!rcbQ->numWCBs )
          continue ; 
        pthread_mutex_lock(rcbQ->lock);
        if ( rcbQ->numWCBs + rcbQ->numRCBs > rcbQ->sizeR )
        {
          void *ptr;
          size_t s ;
          for ( s=rcbQ->sizeR ; s<(rcbQ->numWCBs+rcbQ->numRCBs) ; s +=1024 ) ; // empty body
          s *= sizeof(ismStore_commitCBinfo_t);
          ptr = ism_common_realloc(ISM_MEM_PROBE(ism_memory_store_misc,6),rcbQ->rCBs,s) ; 
          if (!ptr )
          {
            TRACE(1,"realloc failed for %lu bytes!\n",s);
            pthread_mutex_unlock(rcbQ->lock);
            continue ; 
          }
          else
          {
            rcbQ->rCBs = ptr ; 
            rcbQ->sizeR = s / sizeof(ismStore_commitCBinfo_t) ; 
            TRACE(5,"rcbQ->sizeR for thread %u has been increased to %u\n",i,rcbQ->sizeR);
          }
        }
        memcpy(rcbQ->rCBs + rcbQ->numRCBs, rcbQ->wCBs, rcbQ->numWCBs*sizeof(ismStore_commitCBinfo_t)) ; 
        rcbQ->numRCBs += rcbQ->numWCBs ;
        pthread_cond_signal(rcbQ->cond) ; 
        pthread_mutex_unlock(rcbQ->lock);
        pInfo->numWCBs-= rcbQ->numWCBs ;
        __sync_add_and_fetch(&pInfo->numRCBs,rcbQ->numWCBs) ; 
        rcbQ->numWCBs = 0 ; 
      }
      if (!pInfo->rcbHWM && pInfo->numRCBs > RCB_HWM )
      {
        pthread_mutex_lock(pInfo->lock);
        if (!pInfo->rcbHWM && pInfo->numRCBs > RCB_HWM && !pInfo->stopCB)
        {
          ismStore_memJob_t job;
          pInfo->rcbHWM = 1 ; 
          memset(&job, '\0', sizeof(job));
          job.JobType = StoreJob_UserEvent;
          job.Event.EventType = ISM_STORE_EVENT_CBQ_ALERT_ON ; 
          ism_store_memAddJob(&job);
          TRACE(5,"Raising event ISM_STORE_EVENT_CBQ_ALERT_ON ;  numRCBs %u , lwm/hwm %u %u\n",pInfo->numRCBs,RCB_LWM,RCB_HWM);
          pthread_mutex_unlock(pInfo->lock);
          for ( i=0 ; i<pInfo->numTHrx ; i++ )
          {
            pthread_mutex_lock(pInfo->rcbQ[i].lock);
            memset(&pInfo->rcbQ[i].stats,0,sizeof(cbStats));
            //We force stats on as we are not keeping up with callbacks
            pInfo->rcbQ[i].stats.mode = ISM_STORE_ASYNCCBSTATSMODE_FULL;
            pInfo->rcbQ[i].stats.nextTime = ism_common_readTSC() + 5e0 ;
            pthread_mutex_unlock(pInfo->rcbQ[i].lock);
          }
        }
        else
        pthread_mutex_unlock(pInfo->lock);
      }
    }
    pInfo->gotWork = 0 ; 
    pthread_mutex_lock(rdLock);
    for (i=0, l=-1, m=0, n=0 ; m<ismStore_memGlobal.StreamsCount ; i++)
    {
      j = (last+i)%ismStore_memGlobal.StreamsSize ; 
      if ((pStream = ismStore_memGlobal.pStreams[j]) != NULL)
      {
        m++ ; 
        ismSTORE_MUTEX_LOCK(&pStream->Mutex) ;
        pPersist = pStream->pPersist;
       #if USE_STATS
        NN.numFullCB   += pPersist->numFullCB ; 
        NN.numFullBuff += pPersist->numFullBuff ; 
        NN.numCommit   += pPersist->numCommit   ; 
        NN.numComplete += pPersist->numComplete ; 
        NN.numPackets  += pPersist->numPackets  ; 
        NN.numBytes    += pPersist->numBytes    ; 
        memset(&pPersist->numFullCB, 0, 6*sizeof(uint32_t)) ; 
        if ( ss )
        {
          double ncb = pPersist->statCBs ; 
          pPersist->statCBs = 0 ; 
          TRACE(5,"_dbg_PS stats: %f CBPS for stream %u serviced by asyncThread %u and haTxThread %u\n",ncb/dt,pStream->hStream,pPersist->indRx,pPersist->indTx);
        }
       #endif
        if ( (pPersist->State & PERSIST_STATE_WORKING) )
        {
          pPersist->State &= ~PERSIST_STATE_WORKING ; 
          if ( pInfo->useSigTh )
          {
            sigSig++;
            pPersist->State |=  PERSIST_STATE_DONE ;
          }
          else
          {
           #if USE_STATS
            double cs = ism_common_readTSC() ; 
           #endif
            if ( pStream->pPersist->Waiting ) ismSTORE_COND_BROADCAST(&pStream->Cond);
           #if USE_STATS
            tS += ism_common_readTSC() - cs ; 
           #endif
          }
        }
        if ( (pPersist->State & PERSIST_STATE_PENDING) )
        {
          if ( batch > pPersist->BuffLen && 
              (j == ismStore_memGlobal.hInternalStream || !ismSTORE_HASSTANDBY || pPersist->Buf1Len + pPersist->BuffLen <= pPersist->BuffSize) )
          {
            int ok=1;
            if ( pPersist->NumCBs )
            {
              int ind = pStream->pPersist->indRx ; 
              rcbQueue_t *rcbQ = &pInfo->rcbQ[ind] ; 
              if ( rcbQ->numWCBs + pPersist->NumCBs > rcbQ->sizeW )
              {
                void *ptr;
                size_t s ;
                for ( s=rcbQ->sizeW ; s<(rcbQ->numWCBs+pPersist->NumCBs) ; s +=1024 ) ; // empty body
                s *= sizeof(ismStore_commitCBinfo_t);
                ptr = ism_common_realloc(ISM_MEM_PROBE(ism_memory_store_misc,7),rcbQ->wCBs,s) ; 
                if (!ptr )
                {
                  TRACE(1,"realloc failed for %lu bytes!\n",s);
                  if ( l<0 ) l = j ;
                  ok = 0 ; 
                }
                else
                {
                  rcbQ->wCBs = ptr ; 
                  rcbQ->sizeW = s / sizeof(ismStore_commitCBinfo_t) ; 
                  TRACE(5,"rcbQ->sizeW for thread %u has been increased to %u\n",ind,rcbQ->sizeW);
                }
              }
              if ( ok )
              {
                memcpy(rcbQ->wCBs + rcbQ->numWCBs, pPersist->CBs, pPersist->NumCBs * sizeof(ismStore_commitCBinfo_t)) ; 
                rcbQ->numWCBs += pPersist->NumCBs ; 
                pInfo->numWCBs += pPersist->NumCBs ; 
                pInfo->totWCBs += pPersist->NumCBs ; 
                pPersist->statCBs += pPersist->NumCBs ; 
                pPersist->NumCBs = 0 ; 
              }
            }
            if ( ok )
            {
              memcpy(tBuff, pPersist->Buff, pPersist->BuffLen) ; 
              tBuff += pPersist->BuffLen ; 
              pPersist->State &= ~PERSIST_STATE_PENDING ; 
              pPersist->State |=  PERSIST_STATE_WORKING ; 
              batch -= pPersist->BuffLen ; 
              n += pPersist->NumSTs  ; 
              pPersist->NumSTs = 0 ; 
              if ( ismSTORE_HASSTANDBY && j != ismStore_memGlobal.hInternalStream )
              {
                int ind = pStream->pPersist->indTx ; 
                TRACE(9,"persistHA: memcpy(pPersist->Buf1 + pPersist->Buf1Len, pPersist->Buff, pPersist->BuffLen), %u %u %u\n",j,pPersist->Buf1Len, pPersist->BuffLen) ; 
                if ( pPersist->lastSTflags )
                {
                  clrNoAck(*pPersist->lastSTflags) ; 
                  pPersist->lastSTflags = NULL ; 
                }
                if ( pPersist->Buf1Len )
                {
                  memcpy(pPersist->Buf1 + pPersist->Buf1Len, pPersist->Buff, pPersist->BuffLen) ;
                  pPersist->Buf1Len += pPersist->BuffLen ; 
                }
                else
                {
                  void *ptr = pPersist->Buf1 ; 
                  pPersist->Buf1 = pPersist->Buff ; 
                  pPersist->Buff = ptr ; 
                  pPersist->Buf1Len = pPersist->BuffLen ; 
                }
                pInfo->sndT[ind].sigHA++ ; 
                sigHA++ ; 
              }
              else
                pPersist->AckSqn = pPersist->MsgSqn ; 
              pPersist->BuffLen = 0 ; 
              if ( pStream->pPersist->Waiting ) ismSTORE_COND_BROADCAST(&pStream->Cond);
            }
          }
          else
          {
            if ( l<0 ) l = j ;
            pInfo->gotWork++ ; 
          }
        }
        ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);
      }
    }
    pthread_mutex_unlock(rdLock);
    if ( sigHA )
    {
      haSendThread_t *sndT;
      for ( i=0 ; i<pInfo->numTHtx ; i++ )
      {
        sndT = &pInfo->sndT[i] ; 
        if ( sndT->sigHA )
        {
          pthread_mutex_lock(sndT->lock);
          sndT->haWorkS = 1 ; 
          pthread_cond_signal(sndT->cond) ; 
          pthread_mutex_unlock(sndT->lock);
          sndT->sigHA = 0 ;  
        }
      }
    }
    if ( sigSig )
    {
      pthread_mutex_lock(pInfo->sigLock);
      pInfo->sigWork++ ; 
      pthread_cond_broadcast(pInfo->sigCond) ; 
      pthread_mutex_unlock(pInfo->sigLock);
    }
    last = (l>=0) ? l : 0 ; 
   #if USE_STATS
    tT += ism_common_readTSC() - ct ; 
   #endif
    if ( !n )
    {
      if ( pInfo->numWCBs ) 
        continue ; 
     #if USE_STATS
      ct = ism_common_readTSC() ; 
     #endif
      pthread_mutex_lock(pInfo->lock) ; 
      if ( pInfo->iState|pInfo->jState )
      {
        pInfo->go2Work = 0 ; 
        pthread_mutex_unlock(pInfo->lock) ; 
        su_sleep(100,SU_MIC) ; 
        goto cont ; 
      }
      n = FAST_WAIT_ROUNDS ; 
      while ( !pInfo->goDown && !pInfo->go2Work && !pInfo->genClosed && !pInfo->stFull )
      {
        if ( pInfo->gotWork )
        {
          sched_yield(); 
          break ; 
        }
        if ( n )
        {
          pthread_mutex_unlock(pInfo->lock) ; 
          sched_yield() ; 
          pthread_mutex_lock(pInfo->lock) ; 
          n-- ; 
        }
        else
          ism_common_cond_timedwait(pInfo->cond, pInfo->lock, &reltime, 1);
      }
      pInfo->go2Work = 0 ; 
      pthread_mutex_unlock(pInfo->lock) ; 
     cont:
     #if USE_STATS
      if ( t0 ) tW += ism_common_readTSC() - ct ; 
     #endif
      ism_common_going2work();
      continue ; 
    }
    ism_common_going2work();
   #if USE_STATS
    if (!t0 ) t0 = ism_common_readTSC() ; 
    ct = ism_common_readTSC() ; 
   #endif
    pF = &pInfo->pFiles[pInfo->curI][pInfo->curJ] ; 
    fd = pF->ST_fd ; 
    if ( fd != pfd )
    {
      TRACE(5,"%s: Start persisting STs with cid %u to check point set: XX_%u_%u\n",__FUNCTION__,pInfo->PState[1].cycleId,pInfo->curI,pInfo->curJ);
      pfd = fd ; 
    }
    pInfo->nSTs = n ; 
    batch = ((tBuff - hBuff) + sizeof(wTail_t) + psm1) & mask ; 
    wHead->ts = ism_common_currentTimeNanos() ; 
    wHead->cid = pInfo->PState[1].cycleId ; 
    wHead->len = batch ; 
    wHead->nst = pInfo->nSTs ; 
    wTail = hBuff + batch - sizeof(wTail_t) ; 
    wTail->ts  = wHead->ts ; 
    wTail->cid = wHead->cid ; 
    wTail->len = wHead->len ; 

   #if USE_STATS
    tM += ism_common_readTSC() - ct ; 
    ct = ism_common_readTSC() ; 
   #endif
    if ( dummyIOtime )
    {
      double et,dt;
      dt = dummyIOtime ; 
      et = ism_common_readTSC() + dt; 
      while ( dt > 1e-4 )
      {
        su_sleep(100,SU_MIC) ; 
        dt -= 1e-4 ; 
      }
      while ( et > ism_common_readTSC() )
        sched_yield();
    }
    else
    {
      pF->ST_off += batch ; 
      if ( pF->ST_off > pF->ST_HWM && !pInfo->jState && !pInfo->stFull )
      {
        pInfo->stFull = 1 ; 
      }
      if ( ism_store_persistIO(fd, pInfo->wBuff, batch, 0) != batch )
      {
        ism_store_persistFatal(ismSTORE_STATE_DISKERROR , __LINE__) ; 
        break ; 
      }
    }
   #if USE_STATS
    tO += ism_common_readTSC() - ct ; 
   #endif
    pInfo->needCP = 1 ; 

   #if USE_STATS
   {
    #define DKB (1024e0)
    #define DMB (1048576e0)
    #define DGB (1073741824e0)
     double ct,dt;
     pStats->nS += pInfo->nSTs ; 
     pStats->nB += batch ; 
     pStats->nW++ ; 
     if ( pStats->nT < (ct=ism_common_readTSC()) )
     {
       off_t off = lseek(fd, 0, SEEK_CUR);
       dt = ct - pStats->nT + 1e1;
       tG = ism_common_readTSC() - t0 ; 
       TRACE(1,"_dbg_S stats: off=%lu, rates: %f MBPS, %f KBPW, %f KTPS, %f KWPS, %f KAPS, %f TPW, tI=%f, tJ=%f, tW=%f, tO=%f, tT=%f, tM=%f, tS=%f\n",
               off,pStats->nB/dt/DMB,pStats->nB/pStats->nW/DKB,pStats->nS/dt/DKB,pStats->nW/dt/DKB,pStats->nA/dt/DKB,pStats->nS/pStats->nW,tI/tG,tJ/tG,tW/tG,tO/tG,tT/tG,tM/tG,tS/tG);
       memset(pStats,0,sizeof(pStats));
       t0=tG=tI=tJ=tW=tO=tT=tM=tS=0e0 ; 
       TRACE(1,"_dbg_S stats: rates: %f FullCB, %f FullBuff, %f Commit, %f CompleteST, %f Packets, %f packetSize\n",NN.numFullCB/dt,NN.numFullBuff/dt,NN.numCommit/dt,NN.numComplete/dt,NN.numPackets/dt,(double)NN.numBytes/NN.numPackets);
       memset(&NN,0,sizeof(NN));
       pStats->nT = ct + 1e1;
     }
   } 
   #endif
  }

  close(pInfo->PState_fd) ; 
  {
    for ( j=0 ; j<2 && rc==StoreRC_OK ; j++ )
    {
      for ( i=0 ; i<2 && rc==StoreRC_OK; i++ )
      {
        pF = &pInfo->pFiles[i][j] ; 
       #if USE_FLOCK
        ism_store_flock(pF->ST_fd, FLOCK_UN|FLOCK_NB) ; 
       #endif
        close(pF->ST_fd) ; 
      }
    }
  }
  ism_common_backHome();

  pthread_mutex_lock(pInfo->lock) ; 
  pInfo->thUp-- ;
  pthread_mutex_unlock(pInfo->lock) ; 
  TRACE(5, "The %s thread has stopped\n", __FUNCTION__);

  return NULL;
}


static void ism_store_persistHandleGenTran(void)
{
  ismStore_memMgmtHeader_t *pMgmtHeader;
  ismStore_memGenHeader_t *pGenHeader;
  int rc;

  do
  {
    if ( pInfo->iState == 1 )
    {
      ismStore_memJob_t job;
      if ( !pInfo->storeLocked ) pInfo->storeLocked = ism_store_memLockStore(0,LOCKSTORE_CALLER_PRST) ; 
      if ( !pInfo->storeLocked )
      {
        if (!pInfo->lwTO )
             pInfo->lwTO = ism_common_readTSC() + LOCK_WAIT_TO ; 
        if ( pInfo->lwTO < ism_common_readTSC() ) 
        {
          if ( ++pInfo->lwAttempts > LOCK_MAX_ATTEMPTS )
          {
            TRACE(1,"!!! Failed to lock the store after %u attempts !!! , shuting down the server !!! \n",pInfo->lwAttempts);
            su_sleep(10,SU_MIL) ; 
            ism_common_shutdown(1);
          }
          else
          {
            ism_common_stack_trace(0);
            ism_store_memUnlockStore(LOCKSTORE_CALLER_PRST) ; 
            TRACE(1,"Attempt %u to lock the store timed out after %f secs.  Will postpone the genTran and retry.\n",pInfo->lwAttempts,LOCK_WAIT_TO);
            pInfo->lwTO = 0e0 ;
            pInfo->iState = 0 ; 
            TRACE(5,"%s: genTranPhase setting iState to %u\n",__FUNCTION__,pInfo->iState);
            pInfo->lwRT = ism_common_readTSC() + LOCK_WAIT_RT ; 
          }
        }
        else
        if ( !pInfo->lwRaiseEvent && pInfo->lwTO < ism_common_readTSC() +(LOCK_WAIT_TO/2) )
        {
          memset(&job, '\0', sizeof(job));
          job.JobType = StoreJob_UserEvent;
          job.Event.EventType = ISM_STORE_EVENT_CBQ_ALERT_ON ; 
          ism_store_memAddJob(&job);
          TRACE(5,"Raising event ISM_STORE_EVENT_CBQ_ALERT_ON while trying to lock the store\n");
          pInfo->lwRaiseEvent = 1 ; 
        }
        break ; 
      }
      if ( pInfo->lwRaiseEvent )
      {
        memset(&job, '\0', sizeof(job));
        job.JobType = StoreJob_UserEvent;
        job.Event.EventType = ISM_STORE_EVENT_CBQ_ALERT_OFF; 
        ism_store_memAddJob(&job);
        TRACE(5,"Raising event ISM_STORE_EVENT_CBQ_ALERT_OFF after locking the store\n");
        pInfo->lwRaiseEvent = 0 ; 
      }
      pInfo->lwTO = 0e0 ; 
      pInfo->lwRT = 0e0 ; 
      pInfo->lwAttempts = 0 ; 
      if ( pInfo->gotWork || pInfo->go2Work )
        break ; 
      if ( --pInfo->genClosed < 0 )
      {
        TRACE(1,"Should not be here!!! pInfo->genClosed < 0 (%d), will reset to 0\n",pInfo->genClosed) ; 
        pInfo->genClosed = 0 ; 
      }
      pInfo->iState = 2 ;
      pInfo->stFull = 0 ;   // (in case it was set while the store was locking)
      TRACE(5,"%s: genTranPhase setting iState to %u\n",__FUNCTION__,pInfo->iState);
    }
    if ( pInfo->iState == 2 )
    {
      pMgmtHeader= (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress;
      pGenHeader = (ismStore_memGenHeader_t *)ismStore_memGlobal.InMemGens[1-pInfo->curI].pBaseAddress;
      if ( pGenHeader->State != ismSTORE_GEN_STATE_ACTIVE ||
           pMgmtHeader->ActiveGenId != pGenHeader->GenId  ||
           pMgmtHeader->ActiveGenIndex != 1-pInfo->curI   ||
           pInfo->writeGenMsg != 3 )
        break ; 
      pInfo->writeGenMsg = 0;
      pInfo->iState = 3 ; 
      TRACE(5,"%s: genTranPhase setting iState to %u\n",__FUNCTION__,pInfo->iState);
    }
    if ( pInfo->iState == 3 )
    {
      if ( (rc = ism_storePersist_prepareCP(1-pInfo->curI,pInfo->curJ)) != StoreRC_OK )
      {
        ism_store_persistFatal(ismSTORE_STATE_ALLOCERROR, __LINE__) ; 
        break ; 
      }
      if ( lseek(pInfo->pFiles[1-pInfo->curI][pInfo->curJ].ST_fd, 0, SEEK_SET) < 0 )
      {
        ism_store_persistFatal(ismSTORE_STATE_DISKERROR , __LINE__) ; 
        break ; 
      }
      pInfo->pFiles[1-pInfo->curI][pInfo->curJ].ST_off = 0 ; 
      pInfo->PState[0].cycleId = pInfo->PState[1].cycleId ;
      pInfo->PState[0].recLen = sizeof(persistState_t) ;
      pInfo->PState[0].startGen = pInfo->curI ; 
      pInfo->PState[0].genTr = 1 ; 
      pInfo->PState[0].startFile[0] = pInfo->curJ ; 
      pInfo->PState[0].startFile[1] = pInfo->curJ ; 
      pInfo->PState[0].coldStart = 0 ; 
      pInfo->PState[0].isStandby = 0 ; 
      if ( ism_store_persistWritePState(__LINE__) < 0 )
        break ; 
      INC_CID(pInfo->PState[1].cycleId) ;
      pInfo->curI ^= 1 ; 
      pInfo->fileTranAllowed = 1 ; 
      ism_store_memUnlockStore(LOCKSTORE_CALLER_PRST) ; 
      pInfo->storeLocked = 0 ; 
      pInfo->iState = 4 ; 
      TRACE(5,"%s: genTranPhase setting iState to %u\n",__FUNCTION__,pInfo->iState);
    }
    if ( pInfo->iState == 4 )
    {
      if ( (rc = ism_storePersist_writeCP()) != StoreRC_OK )
      {
        ism_store_persistFatal(ismSTORE_STATE_ALLOCERROR , __LINE__) ; 
        break ; 
      }
      pInfo->iState = 5 ; 
      TRACE(5,"%s: genTranPhase setting iState to %u\n",__FUNCTION__,pInfo->iState);
    }
    if ( pInfo->iState == 5 )
    {
      if ( pInfo->fileWriteState[0] == 1 && pInfo->fileWriteState[1] == 1 )
      {
        {
          pInfo->iState = 6 ; 
          TRACE(5,"%s: genTranPhase setting iState to %u\n",__FUNCTION__,pInfo->iState);
          //su_sleep(3000);
        }
      }
      else
      if ( pInfo->fileWriteState[0] ==-1 || pInfo->fileWriteState[1] ==-1 )
      {
        TRACE(1,"%s: pInfo->fileWriteState[0] ==-1 || pInfo->fileWriteState[1] ==-1 ; %u %u\n",__FUNCTION__,
                     pInfo->fileWriteState[0],        pInfo->fileWriteState[1]) ; 
        ism_store_persistFatal(ismSTORE_STATE_DISKERROR , __LINE__) ; 
        break ; 
      }
    }
    if ( pInfo->iState == 6 )
    {
      pInfo->PState[0].cycleId = pInfo->PState[1].cycleId ;
      pInfo->PState[0].recLen = sizeof(persistState_t) ;
      pInfo->PState[0].startGen = pInfo->curI ; 
      pInfo->PState[0].genTr = 0 ; 
      pInfo->PState[0].startFile[0] = pInfo->curJ ; 
      pInfo->PState[0].startFile[1] = pInfo->curJ ; 
      pInfo->PState[0].coldStart = 0 ; 
      pInfo->PState[0].isStandby = 0 ; 
      if ( ism_store_persistWritePState(__LINE__) < 0 )
        break ; 
      pInfo->iState = 0 ; 
      TRACE(5,"%s: genTranPhase setting iState to %u\n",__FUNCTION__,pInfo->iState);
    }
  } while(0) ; 
}

static void ism_store_persistHandleFileFlip(void)
{
  int rc;

  do
  {
    if ( pInfo->jState == 1 )
    {
      if ( !pInfo->storeLocked ) pInfo->storeLocked = ism_store_memLockStore(0,LOCKSTORE_CALLER_PRST) ; 
      if ( !pInfo->storeLocked )
      {
        if (!pInfo->lwTO )
             pInfo->lwTO = ism_common_readTSC() + LOCK_WAIT_TO ; 
        if ( pInfo->lwTO < ism_common_readTSC() ) 
        {
          persistFiles_t *pF = &pInfo->pFiles[pInfo->curI][pInfo->curJ] ; 
          ism_common_stack_trace(0);
          ism_store_memUnlockStore(LOCKSTORE_CALLER_PRST) ; 
          TRACE(1,"An attempt to lock the store timed out after %f secs.  Will postpone the FileFlip and retry.  Current ST file size %lu, HWM %lu\n",
                LOCK_WAIT_TO,pF->ST_off,pF->ST_HWM);
          pInfo->lwTO = 0e0 ;
          pInfo->jState = 0 ; 
          pInfo->stFull = 1 ; 
          TRACE(5,"%s: stFullPhase setting jState to %u\n",__FUNCTION__,pInfo->jState);
          pInfo->lwRT = ism_common_readTSC() + LOCK_WAIT_RT ; 
        }
        break ; 
      }
      pInfo->lwTO = 0e0 ; 
      pInfo->lwRT = 0e0 ; 
      if ( pInfo->genClosed )
      {
        TRACE(5,"%s: stFullPhase is aborted in favor of genTranPhase, jState=%u.\n",__FUNCTION__,pInfo->jState);
        pInfo->jState = 0 ; 
        pInfo->iState = 1 ; 
        TRACE(5,"%s: genTranPhase setting iState to %u\n",__FUNCTION__,pInfo->iState);
        break ; 
      }
      pInfo->jState = 2 ; 
      TRACE(5,"%s: stFullPhase setting jState to %u\n",__FUNCTION__,pInfo->jState);
    }
    if ( pInfo->jState == 2 )
    {
      if ( pInfo->gotWork || pInfo->go2Work )
        break ; 
      if ( (rc = ism_storePersist_prepareCP(pInfo->curI,1-pInfo->curJ)) != StoreRC_OK )
      {
        ism_store_persistFatal(ismSTORE_STATE_ALLOCERROR , __LINE__) ; 
        break ; 
      }
      pInfo->jState = 3 ; 
      TRACE(5,"%s: stFullPhase setting jState to %u\n",__FUNCTION__,pInfo->jState);
    }
    if ( pInfo->jState == 3 )
    {
      if ( (rc = ism_storePersist_writeCP()) != StoreRC_OK )
      {
        ism_store_persistFatal(ismSTORE_STATE_ALLOCERROR , __LINE__) ; 
        break ; 
      }
      pInfo->jState = 4 ; 
      TRACE(5,"%s: stFullPhase setting jState to %u\n",__FUNCTION__,pInfo->jState);
    }
    if ( pInfo->jState == 4 )
    {
      if ( pInfo->fileTranAllowed != 1 )
        break ; 
      pInfo->jState = 5 ; 
      TRACE(5,"%s: stFullPhase setting jState to %u\n",__FUNCTION__,pInfo->jState);
    }
    if ( pInfo->jState == 5 )
    {
      INC_CID(pInfo->PState[1].cycleId) ;
      pInfo->curJ ^= 1 ; 
      if ( lseek(pInfo->pFiles[pInfo->curI][pInfo->curJ].ST_fd, 0, SEEK_SET) < 0 )
      {
        ism_store_persistFatal(ismSTORE_STATE_DISKERROR , __LINE__) ; 
        break ; 
      }
      pInfo->pFiles[pInfo->curI][pInfo->curJ].ST_off = 0 ; 
      pInfo->fileTranAllowed = 0 ; 
      ism_store_memUnlockStore(LOCKSTORE_CALLER_PRST) ; 
      pInfo->storeLocked = 0 ; 
      pInfo->jState = 6 ; 
      TRACE(5,"%s: stFullPhase setting jState to %u\n",__FUNCTION__,pInfo->jState);
    }
    if ( pInfo->jState == 6 )
    {
      if ( pInfo->fileWriteState[0] == 1 && pInfo->fileWriteState[1] == 1 )
      {
        pInfo->jState = 7 ; 
        TRACE(5,"%s: stFullPhase setting jState to %u\n",__FUNCTION__,pInfo->jState);
      }
      else
      if ( pInfo->fileWriteState[0] ==-1 || pInfo->fileWriteState[1] ==-1 )
      {
        TRACE(1,"%s: pInfo->fileWriteState[0] ==-1 || pInfo->fileWriteState[1] ==-1 ; %u %u\n",__FUNCTION__,
                     pInfo->fileWriteState[0],        pInfo->fileWriteState[1]) ; 
        ism_store_persistFatal(ismSTORE_STATE_DISKERROR , __LINE__) ; 
        break ; 
      }
    }
    if ( pInfo->jState == 7 )
    {
      pInfo->PState[0].cycleId = pInfo->PState[1].cycleId ;
      pInfo->PState[0].recLen = sizeof(persistState_t) ;
      pInfo->PState[0].startGen = pInfo->curI ; 
      pInfo->PState[0].genTr = 0 ; 
      pInfo->PState[0].startFile[0] = pInfo->curJ ; 
      pInfo->PState[0].startFile[1] = pInfo->curJ ; 
      pInfo->PState[0].coldStart = 0 ; 
      pInfo->PState[0].isStandby = 0 ; 
      if ( ism_store_persistWritePState(__LINE__) < 0 )
        break ; 
      pInfo->fileTranAllowed = 1 ; 
      pInfo->jState = 0 ; 
      TRACE(5,"%s: stFullPhase setting jState to %u\n",__FUNCTION__,pInfo->jState);
    }
  } while(0) ; 
}

static inline char *ism_store_getBaseAddr(ismStore_GenId_t genId)
{
  ismStore_memGeneration_t *pGen;
  ismStore_memGenMap_t *pGenMap = ismStore_memGlobal.pGensMap[genId] ;
  if ( pGenMap && (pGen=pGenMap->pGen) )
    return pGen->pBaseAddress ;
  return NULL;
}

int ism_storePersist_stop(void)
{
  int rc=StoreRC_OK, i;
  if ( pInfo->init > PERSIST_STATE_INITIALIZED )
  {
    pthread_mutex_lock(pInfo->lock) ; 
    while ( pInfo->iState || pInfo->jState )
    {
      pInfo->go2Work = 1 ; 
      pthread_cond_signal(pInfo->cond);
      pthread_mutex_unlock(pInfo->lock) ; 
      su_sleep(1,SU_MIL) ; 
      pthread_mutex_lock(pInfo->lock) ; 
    }
    pInfo->go2Work = 1 ; 
    pInfo->goDown  = 1 ; 
    pthread_cond_signal(pInfo->cond);
    pthread_mutex_unlock(pInfo->lock) ; 
    for ( i=0 ; i<pInfo->numTHrx ; i++ )
    {
      rcbQueue_t *rcbQ = &pInfo->rcbQ[i] ; 
      pthread_mutex_lock(rcbQ->lock);
      pthread_cond_signal(rcbQ->cond) ; 
      pthread_mutex_unlock(rcbQ->lock);
    }
    if ( pInfo->useSigTh )
    {
      pthread_mutex_lock(pInfo->sigLock);
      pthread_cond_broadcast(pInfo->sigCond) ; 
      pthread_mutex_unlock(pInfo->sigLock);
    }
    if ( pInfo->enableHA )
    {
      haSendThread_t *sndT;
      for ( i=0 ; i<pInfo->numTHtx ; i++ )
      {
        sndT = &pInfo->sndT[i] ; 
        pthread_mutex_lock(sndT->lock);
        sndT->haWorkS = 0 ; 
        pthread_cond_signal(sndT->cond) ; 
        pthread_mutex_unlock(sndT->lock);
      }
      pthread_mutex_lock(pInfo->haLockR);
      pthread_cond_signal(pInfo->haCondR) ; 
      pthread_mutex_unlock(pInfo->haLockR);
    }
    pthread_mutex_lock(pInfo->lock) ; 
    while ( pInfo->thUp )
    {
      pthread_mutex_unlock(pInfo->lock) ; 
      su_sleep(1,SU_MIL) ; 
      pthread_mutex_lock(pInfo->lock) ; 
    }
    pthread_mutex_unlock(pInfo->lock) ; 
  }
  return rc ; 
}

int ism_storePersist_getState(void)
{
  return pInfo->init;
}

int ism_storePersist_term(int enforcCP)
{
  int rc=StoreRC_OK, i;
  if ( pInfo->init > PERSIST_STATE_INITIALIZED )
  {
    ism_storePersist_stop() ; 
    pthread_mutex_lock(pInfo->lock) ; 
    pInfo->goDown  = 0 ; 
    pthread_mutex_unlock(pInfo->lock) ; 
    if ( pInfo->needCP )
      rc = ism_storePersist_createCP(0) ; 
  }
  else if ( pInfo->init > PERSIST_STATE_UNINITIALIZED && enforcCP && pInfo->needCP && ismStore_memGlobal.fMemValid )
    rc = ism_storePersist_createCP(0) ;
  if ( pInfo->init )
  {
    pthread_mutex_lock(pInfo->lock) ; 
    pInfo->init = PERSIST_STATE_UNINITIALIZED;
    pthread_mutex_unlock(pInfo->lock) ; 
    closedir(pInfo->di->pdir) ; 
    if (pInfo->di->path) ism_common_free(ism_memory_store_misc,pInfo->di->path) ;
    ism_common_free_memaligned(ism_memory_store_misc,pInfo->wBuff) ;
    pthread_mutex_destroy(pInfo->lock);
    pthread_cond_destroy(pInfo->cond);
    pthread_mutex_destroy(pInfo->sigLock);
    pthread_cond_destroy(pInfo->sigCond);
    pthread_mutex_destroy(pInfo->haLockR);
    pthread_cond_destroy(pInfo->haCondR);

    for ( i=0 ; i<pInfo->numTHrx ; i++ )
    {
      rcbQueue_t *rcbQ = &pInfo->rcbQ[i] ; 
      pthread_mutex_destroy(rcbQ->lock);
      pthread_cond_destroy (rcbQ->cond);
      if ( rcbQ->rCBs && rcbQ->sizeR )
      {
        ism_common_free(ism_memory_store_misc,rcbQ->rCBs) ; 
        rcbQ->rCBs = NULL ; 
        rcbQ->sizeR = 0 ; 
      }
      if ( rcbQ->wCBs && rcbQ->sizeW )
      {
        ism_common_free(ism_memory_store_misc,rcbQ->wCBs) ; 
        rcbQ->wCBs = NULL ; 
        rcbQ->sizeW = 0 ; 
      }
      if ( rcbQ->pCBs && rcbQ->sizeP )
      {
        ism_common_free(ism_memory_store_misc,rcbQ->pCBs) ; 
        rcbQ->pCBs = NULL ; 
        rcbQ->sizeP = 0 ; 
      }
    }
    for ( i=0 ; i<pInfo->numTHtx ; i++ )
    {
      pthread_mutex_destroy(pInfo->sndT[i].lock);
      pthread_cond_destroy(pInfo->sndT[i].cond);
    }
    for ( i=0 ; i<pInfo->numRW ; i++ )
      pthread_mutex_destroy(&pInfo->rwLocks[i]);
  }
 #if USE_HISTO
  hist_free(cbSi[0]);
  hist_free(cbSi[1]);
  hist_free(cbSi[2]);
  hist_free(cbSi[3]);
  hist_free(cbSi[4]);
  hist_free(cbSi[5]);
 #endif
  return rc ; 
}

static int ism_storePersist_openSTfiles(int flags)
{
  int rc = ISMRC_OK ; 
  for ( int i=0 ; i<2 ; i++ )
  {
    for ( int j=0 ; j<2 ; j++ )
    {
      int fd=0;
      persistFiles_t *pF = &pInfo->pFiles[i][j] ; 
      char *buff = pInfo->wBuff ; 
      if ( flags&1 )
      {
        size_t l = sizeof(ismStore_memMgmtHeader_t) ; 
        TRACE(5,"%s: Header of %s\n",__FUNCTION__,pF->CPM_fn);
        if ( (fd = openat(pInfo->di->fdir,pF->CPM_fn,O_RDONLY|O_CLOEXEC,0)) > 0 && read(fd,buff,l) == l  )
          ism_store_persistPrintMgmtHeader(buff,__LINE__);
        if ( flags&2 ) {ismSTORE_CLOSE_FD(fd);} else pF->CPM_fd = fd ; 
        l = sizeof(ismStore_memGenHeader_t) ; 
        TRACE(5,"%s: Header of %s\n",__FUNCTION__,pF->CPG_fn);
        if ( (fd = openat(pInfo->di->fdir,pF->CPG_fn,O_RDONLY|O_CLOEXEC,0)) > 0 && read(fd,buff,l) == l  )
          ism_store_persistPrintGenHeader(buff,__LINE__);
        if ( flags&2 ) {ismSTORE_CLOSE_FD(fd);} else pF->CPG_fd = fd ;
        l = sizeof(wHead_t) ; 
        TRACE(5,"%s: Header of %s\n",__FUNCTION__,pF->ST_fn);
        if ( (fd = openat(pInfo->di->fdir,pF->ST_fn,O_RDONLY|O_CLOEXEC,0)) > 0 )
        {
          if ( read(fd,buff,l) == l  )
          {
            wHead_t *h = (wHead_t *)buff ; 
            TRACE(5,"%s header: ts=%lu, cid=%u, len=%u, nst=%u\n",pF->ST_fn,h->ts,h->cid,h->len,h->nst);
          }
        }
      }
      if ( (flags&4) || ((flags&1) && fd <=0) )
      {
        char *fn = pF->ST_fn ; 
        if ( (flags&4) || ((flags&1) && errno == ENOENT) )
        {
          int f = O_WRONLY | O_CREAT | O_NOATIME | O_CLOEXEC           | O_DIRECT ;
          if ( (fd = openat(pInfo->di->fdir,fn,f,ismStore_memGlobal.PersistedFileMode)) < 0 )
          {
            int f1 = f & ~(O_DIRECT) ; // some virtual appliances do not support O_DIRECT
            if ( (fd = openat(pInfo->di->fdir,fn,f1,ismStore_memGlobal.PersistedFileMode)) < 0 )
            {
              TRACE(1,"Failed to open file %s/%s for persistence, errno=%d (%s).\n",pInfo->di->path,fn,errno,strerror(errno));
              rc = StoreRC_SystemError ; 
              break ; 
            }
            else if ( (flags&4) && !(flags&2) )
            {
              TRACE(3,"%s: O_DIRECT not supported! %s/%s file opened without O_DIRECT: fd=%d\n",__FUNCTION__,pInfo->di->path,fn,pF->ST_fd);
            }
          }
          else if ( (flags&4) && !(flags&2) )
          {
            TRACE(5,"%s: %s/%s file opened: fd=%d\n",__FUNCTION__,pInfo->di->path,fn,pF->ST_fd);
          }

          if ( ftruncate(fd, pInfo->PersistParams->PersistFileSize) != 0 ) {
              TRACE(1,"Failed to truncate %s, errno=%d (%s).\n",fn,errno,strerror(errno));
          }
          ism_storePersist_zeroFile(fd, pInfo->PersistParams->PersistFileSize);
          pF->ST_size = pInfo->PersistParams->PersistFileSize ; 
          pF->ST_HWM  = pF->ST_size - pInfo->wSize ;
         #if USE_FLOCK
          if ( ism_store_flock(pF->ST_fd, FLOCK_EX)<0 ) 
          {
            TRACE(1,"ism_store_flock failed, errno %d (%s)\n",errno,strerror(errno)); 
          }
         #endif
        }
        else if ( (flags&1) && (errno != ENOENT) )
        {
          TRACE(1,"Failed to open file %s/%s for persistence, errno=%d (%s).\n",pInfo->di->path,fn,errno,strerror(errno));
          rc = StoreRC_SystemError ; 
          break ; 
        }
      }
      if ( flags&2 ) {ismSTORE_CLOSE_FD(fd);} else pF->ST_fd = fd ; 
    }
  }
  return rc ; 
}

int ism_storePersist_getStoreSize(char *RootPath, uint64_t *pSize)
{
  int rc = ISMRC_OK, iok=0;
  ismStoe_DirInfo di[1];
  persistFiles_t  pFiles[2][2];
  int   PState_fd ; 
  char  PState_fn[8] ; 
  persistState_t PState[1] ; 
  do
  {
    int i,j;
    if ( !RootPath )
    {
      TRACE(1, "%s: the argument 'RootPath' is NULL.\n", __FUNCTION__);
      rc = StoreRC_BadParameter ; 
      break ; 
    }
    if ( !pSize )
    {
      TRACE(1, "%s: the argument 'pSize' is NULL.\n", __FUNCTION__);
      rc = StoreRC_BadParameter ; 
      break ; 
    }
    rc = ism_storeDisk_initDir(RootPath, di) ; 
    if ( rc != StoreRC_OK )
      break ; 
    iok++ ; 
    su_strlcpy(PState_fn,"PState",8);
    for ( j=0 ; j<2 ; j++ )
    {
      for ( i=0 ; i<2 ; i++ )
      {
        persistFiles_t *pF = &pFiles[i][j] ; 
        snprintf(pF->ST_fn, 8, "ST_%u_%u",i,j);
        snprintf(pF->CPM_fn,8, "CPM_%u_%u",i,j);
        snprintf(pF->CPG_fn,8, "CPG_%u_%u",i,j);
      }
    }
    if ( (PState_fd = openat(di->fdir,PState_fn,O_RDONLY|O_CLOEXEC,0)) < 0 )
    {
      *pSize= 0 ; 
      break ; 
    }
    else
    {
      ssize_t n ; 
      size_t l = sizeof(persistState_t);
      iok++ ;
      if ( (n=read(PState_fd, PState, l)) < 0 )
      {
        TRACE(1,"Failed to read file %s/%s for persistence, errno=%d (%s).\n",di->path,PState_fn,errno,strerror(errno));
        rc = StoreRC_SystemError ; 
        break ; 
      }
      if ( n < offsetof(persistState_t,recLen)+sizeof(uint16_t) || PState[0].recLen != n )
      {
        TRACE(1,"Failed to read file %s/%s for persistence.  read size (%ld) is too small.(%lu, %hu)\n",di->path,PState_fn,n,l,PState[0].recLen);
        rc = StoreRC_SystemError ; 
        break ; 
      }
      if ( PState[0].recLen != l )
      {
        // For future version change handling
      }
      if ( PState[0].coldStart )
      {
        *pSize= 0 ; 
        break ; 
      }
      i = PState[0].startGen ; 
      j = PState[0].startFile[i];
      {
        {
          int fd;
          persistFiles_t *pF = &pFiles[i][j] ; 
          ismStore_memMgmtHeader_t pMgmtHeader[1] ; 
          l = sizeof(ismStore_memMgmtHeader_t) ; 
          if ( (fd = openat(di->fdir,pF->CPM_fn,O_RDONLY|O_CLOEXEC,0)) > 0 && read(fd,pMgmtHeader,l) == l  )
            *pSize = pMgmtHeader->TotalMemSizeBytes ; 
            //  *pSize = pMgmtHeader->HaveData ? pMgmtHeader->TotalMemSizeBytes : 0 ; 
          else
          {
            TRACE(1,"Failed to read file %s/%s for persistence, errno=%d (%s).\n",di->path,pF->CPM_fn,errno,strerror(errno));
            rc = StoreRC_SystemError ; 
          }
          ismSTORE_CLOSE_FD(fd);
        }
      }
    }
  } while(0) ; 
  if ( iok > 1 )
    ismSTORE_CLOSE_FD(PState_fd);
  if ( iok )
  {
    closedir(di->pdir) ; 
    if (di->path) ism_common_free(ism_memory_store_misc,di->path) ;
  }
  return rc ; 
}
int ism_storePersist_init(ismStore_PersistParameters_t *pPersistParams)
{
  int rc=StoreRC_OK ; 
  int i,j,iok=0, oki ; 

  do
  {
    i = ism_common_getIntConfig("Store.dummyCBdelay", 0);
    deliveryDelay = 1e-6 * i ; 
    if ( !pPersistParams )
    {
      TRACE(1, "%s: the argument 'pPersistParams' is NULL.\n", __FUNCTION__);
      rc = StoreRC_BadParameter ; 
      break ; 
    }
    memset(pInfo,0,sizeof(persistInfo_t));
    rc = ism_storeDisk_initDir(pPersistParams->RootPath, pInfo->di) ; 
    if ( rc != StoreRC_OK )
      break ; 
    TRACE(5,"%s: dirInfo: pdir=%p , path=%s , block=%lu, fdir=%d, plen=%d\n",__FUNCTION__,
          pInfo->di->pdir, pInfo->di->path, pInfo->di->block, pInfo->di->fdir, pInfo->di->plen);
    {
      struct dirent *de ; 
      rewinddir(pInfo->di->pdir);
      while ( (de = readdir(pInfo->di->pdir)) )
      {
       #ifdef _DIRENT_HAVE_D_TYPE
        if ( de->d_type && de->d_type != DT_REG ) continue ; 
       #endif
        if ( ism_store_isTmpName(de->d_name) )
        {
          if ( unlinkat(pInfo->di->fdir, de->d_name,0) )
          {
            rc = StoreRC_SystemError ; 
            break ; 
          }
        }
      }
    } 

    iok++ ; 
    pPersistParams->DiskBlockSize = pInfo->di->block ; 
    memcpy(pInfo->PersistParams, pPersistParams, sizeof(ismStore_PersistParameters_t)) ; 
    pInfo->useYield = (pPersistParams->PersistThreadPolicy == 1) ; 
    pInfo->useSigTh = (pPersistParams->PersistThreadPolicy == 2) ; 
    pInfo->enableHA = pPersistParams->EnableHA ; 
    pInfo->di->batch = pPersistParams->TransferBlockSize ;
    if ( pInfo->di->batch < pInfo->di->block )
         pInfo->di->batch = pInfo->di->block ;
    pInfo->di->batch = (pInfo->di->batch+pInfo->di->block-1)/pInfo->di->block*pInfo->di->block ; 
    su_strlcpy(pInfo->PState_fn,"PState",8);
    pInfo->numTHrx = pInfo->PersistParams->PersistAsyncThreads ; 
    if ( pInfo->numTHrx > MAX_ASYNC_THREADs )
         pInfo->numTHrx = MAX_ASYNC_THREADs ;
    pInfo->numTHtx = pInfo->PersistParams->PersistHaTxThreads ;
    if ( pInfo->numTHtx > MAX_ASYNC_THREADs )
         pInfo->numTHtx = MAX_ASYNC_THREADs ;
    RCB_HWM = pInfo->PersistParams->PersistCbHwm ; 
    if ( RCB_LWM > RCB_HWM/4 )
         RCB_LWM = RCB_HWM/4 ;
    for ( j=0 ; j<2 ; j++ )
    {
      for ( i=0 ; i<2 ; i++ )
      {
        persistFiles_t *pF = &pInfo->pFiles[i][j] ; 
        snprintf(pF->ST_fn, 8, "ST_%u_%u",i,j);
        snprintf(pF->CPM_fn,8, "CPM_%u_%u",i,j);
        snprintf(pF->CPG_fn,8, "CPG_%u_%u",i,j);
      }
    }
    if ( pPersistParams->RecoverFromV12 )
    {
      TRACE(3,"RecoverFromV12 is set !!! assuming input persist files are from MS V1.2 !!!\n");
      pInfo->add2len = 0 ; 
      pInfo->add2sid = 0 ; 
      pInfo->add2ptr = 4 ; 
    }
    else
    {
      pInfo->add2len = 4 ; 
      pInfo->add2sid =15 ; // SHORT + LONG + INT + CHAR
      pInfo->add2ptr = 0 ; 
    }
    do
    {
      oki = 0  ; 
      pInfo->wSize = ismStore_memGlobal.StreamsMinCount + pInfo->numTHrx; 
      pInfo->wSize *= 3 * pInfo->PersistParams->StreamBuffSize / 2 ; 
      TRACE(5,"PersistThread write buffer size: %lu %lu\n",pInfo->wSize,pInfo->PersistParams->StreamBuffSize);
      if ( posix_memalign(&pInfo->wBuff, pInfo->di->block, pInfo->wSize) )
      {
        rc = StoreRC_AllocateError ;
        break ; 
      }
      memset(pInfo->wBuff,0,pInfo->wSize);
      oki++ ; 
      if ( pthread_mutex_init(pInfo->lock,NULL) )
      {
        rc = StoreRC_SystemError ; 
        break ; 
      }
      oki++ ; 
      if ( ism_common_cond_init_monotonic(pInfo->cond) )
      {
        rc = StoreRC_SystemError ; 
        break ; 
      }
      oki++ ; 
      if ( pthread_mutex_init(pInfo->sigLock,NULL) )
      {
        rc = StoreRC_SystemError ; 
        break ; 
      }
      oki++ ; 
      if ( ism_common_cond_init_monotonic(pInfo->sigCond) )
      {
        rc = StoreRC_SystemError ; 
        break ; 
      }
      oki++ ; 
      for ( i=0 ; i<pInfo->numTHrx ; i++ )
      {
        if ( pthread_mutex_init(pInfo->rcbQ[i].lock,NULL) )
        {
          rc = StoreRC_SystemError ; 
          break ; 
        }
      }
      if ( i<pInfo->numTHrx )
        break ; 
      oki++ ; 
      for ( i=0 ; i<pInfo->numTHrx ; i++ )
      {
        if ( ism_common_cond_init_monotonic(pInfo->rcbQ[i].cond) )
        {
          rc = StoreRC_SystemError ; 
          break ; 
        }
      }
      if ( i<pInfo->numTHrx )
        break ; 
      oki++ ; 
      if ( pthread_mutex_init(pInfo->haLockR,NULL) )
      {
        rc = StoreRC_SystemError ; 
        break ; 
      }
      oki++ ; 
      if ( ism_common_cond_init_monotonic(pInfo->haCondR) )
      {
        rc = StoreRC_SystemError ; 
        break ; 
      }
      oki++ ; 
      for ( i=0 ; i<pInfo->numTHtx ; i++ )
      {
        if ( pthread_mutex_init(pInfo->sndT[i].lock,NULL) )
        {
          rc = StoreRC_SystemError ; 
          break ; 
        }
      }
      if ( i<pInfo->numTHtx )
        break ; 
      oki++ ; 
      for ( i=0 ; i<pInfo->numTHtx ; i++ )
      {
        if ( ism_common_cond_init_monotonic(pInfo->sndT[i].cond) )
        {
          rc = StoreRC_SystemError ; 
          break ; 
        }
      }
      if ( i<pInfo->numTHtx )
        break ; 
      oki++ ; 
      pInfo->numRW = 4 + pInfo->numTHtx ; 
      for ( i=0 ; i<pInfo->numRW ; i++ )
      {
        if ( pthread_mutex_init(&pInfo->rwLocks[i],NULL) )
        {
          rc = StoreRC_SystemError ; 
          break ; 
        }
      }
      if ( i<pInfo->numRW )
        break ; 
      pInfo->indRW = 1 ; // index 0 is used for incoming callers
      oki++ ; 
    } while(0) ; 
    if ( oki < 12 )
    {
      if ( oki > 11)
        for ( i=0 ; i<pInfo->numRW ; i++ )
          if ( pthread_mutex_destroy(&pInfo->rwLocks[i]) )
      if ( oki > 10)
        for ( i=0 ; i<pInfo->numTHtx ; i++ )
          pthread_cond_destroy(pInfo->sndT[i].cond);
      if ( oki > 9 )
        for ( i=0 ; i<pInfo->numTHtx ; i++ )
          pthread_mutex_destroy(pInfo->sndT[i].lock);
      if ( oki > 8 )
        pthread_cond_destroy(pInfo->haCondR);
      if ( oki > 7 )
        pthread_mutex_destroy(pInfo->haLockR);
      if ( oki > 6 )
        for ( i=0 ; i<pInfo->numTHrx ; i++ )
          pthread_cond_destroy(pInfo->rcbQ[i].cond);
      if ( oki > 5 )
        for ( i=0 ; i<pInfo->numTHrx ; i++ )
          pthread_mutex_destroy(pInfo->rcbQ[i].lock);
      if ( oki > 4 )
        pthread_cond_destroy(pInfo->sigCond);
      if ( oki > 3 )
        pthread_mutex_destroy(pInfo->sigLock);
      if ( oki > 2 )
        pthread_cond_destroy(pInfo->cond);
      if ( oki > 1 )
        pthread_mutex_destroy(pInfo->lock);
      if ( oki > 0 )
        ism_common_free_memaligned(ism_memory_store_misc,pInfo->wBuff);
      break ; 
    }
    if ( (pInfo->PState_fd = openat(pInfo->di->fdir,pInfo->PState_fn,O_RDONLY|O_CLOEXEC,0)) < 0 )
    {
      if ( errno == ENOENT )
      {
        TRACE(5,"Failed to open file %s/%s for persistence, errno=%d (%s).\n",pInfo->di->path,pInfo->PState_fn,errno,strerror(errno));
        pInfo->PState[1].cycleId = 1 ; 
        if ( (rc = ism_storePersist_openSTfiles(6)) )
          break ; 
      }
      else
      {
        TRACE(1,"Failed to open file %s/%s for persistence, errno=%d (%s).\n",pInfo->di->path,pInfo->PState_fn,errno,strerror(errno));
        rc = StoreRC_SystemError ; 
        break ; 
      }
    }
    else
    {
      ssize_t n ; 
      persistState_t *pS = &pInfo->PState[0];
      size_t l = sizeof(persistState_t);
      if ( (n=read(pInfo->PState_fd, pInfo->PState, l)) < 0 )
      {
        TRACE(1,"Failed to read file %s/%s for persistence, errno=%d (%s).\n",pInfo->di->path,pInfo->PState_fn,errno,strerror(errno));
        rc = StoreRC_SystemError ; 
        break ; 
      }
      TRACE(5,"%s: read  PState: cid=%u, curI=%u, curJ=%u, genTr=%u, Cold=%u, len=%u\n",__FUNCTION__,pS->cycleId,pS->startGen,pS->startFile[0],pS->genTr,pS->coldStart,pS->recLen);
      if ( n < offsetof(persistState_t,recLen)+sizeof(uint16_t) || pInfo->PState[0].recLen != n )
      {
        TRACE(1,"Failed to read file %s/%s for persistence.  read size (%ld) is too small.(%lu, %hu)\n",pInfo->di->path,pInfo->PState_fn,n,l,pInfo->PState[0].recLen);
        rc = StoreRC_SystemError ; 
        break ; 
      }
      if ( pInfo->PState[0].recLen != l )
      {
        // For future version change handling
      }
      memcpy(&pInfo->PState[1],&pInfo->PState[0],l);
      pInfo->curI = pInfo->PState[0].startGen ; 
      pInfo->curJ = pInfo->PState[0].startFile[pInfo->curI];
      if ( (rc = ism_storePersist_openSTfiles(3)) )
                  break ; 
                }
    iok++ ; 
  } while(0) ; 
  if ( iok < 2 )
  {
    if ( iok > 0 )
    {
      if (pInfo->di->path) ism_common_free(ism_memory_store_misc,pInfo->di->path) ;
      closedir(pInfo->di->pdir) ; 
    }
  }
  else
  {
    pthread_mutex_lock(pInfo->lock) ; 
    pInfo->init=PERSIST_STATE_INITIALIZED;
    pthread_mutex_unlock(pInfo->lock) ; 
  }
  ismSTORE_CLOSE_FD(pInfo->PState_fd) ; 
  pInfo->needCP = 1 ; 

 #if USE_HISTO
/*
 0 -> CB_DELAY   10 sec / 1 mil (10000 1e-3)
 1 -> ACK_DELAY  10 sec / 1 mil (10000 1e-3)
 2 -> CB_CALL0   10 sec / 10 mil (1000 1e-2)
 3 -> CB_CALL1   10 mil / 10 mic (1000 1e-5)
 4 -> FILL_BUF    2 sec / 100 mic (20000 1e-4)
 5 -> SEND_BUF    2 sec / 100 mic (20000 1e-4)
*/
  cbSi[0] = hist_alloc(10000, 1e-3, pInfo->totWCBs) ; 
  cbSi[1] = hist_alloc(10000, 1e-3, pInfo->totWCBs) ; 
  cbSi[2] = hist_alloc( 1000, 1e-2, pInfo->totWCBs) ; 
  cbSi[3] = hist_alloc( 1000, 1e-5, pInfo->totWCBs) ; 
  cbSi[4] = hist_alloc(20000, 1e-4, pInfo->totWCBs) ; 
  cbSi[5] = hist_alloc(20000, 1e-4, pInfo->totWCBs) ; 
 #endif

  return rc ; 
}


int ism_store_persistReplayFile(persistReplay_t *pReplay)
{
  int rc=StoreRC_OK, cI, cJ, cid ; 
  char *buff, *bptr ; 
  size_t blen, ask, get, nIo=0, nSt=0 ; 
  wHead_t *wH ; 
  wTail_t *wT ; 
  persistFiles_t *pF ; 
  int i,fd , iok ; 

  cI = pReplay->cI ; 
  cJ = pReplay->cJ ; 
  cid= pReplay->cId ; 
  pF = &pInfo->pFiles[cI][cJ] ; 
  iok = 0 ; 
  do
  {
    blen = pInfo->wSize ; 
    if (!(buff = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,16),blen)) )
    {
      rc = StoreRC_AllocateError ;
      break ; 
    }
    iok++ ; 
    if ( (pF->ST_fd = openat(pInfo->di->fdir,pF->ST_fn,O_RDONLY|O_CLOEXEC,0)) < 0 )
    {
      TRACE(1,"Failed to open file %s/%s for persistence, errno=%d (%s).\n",pInfo->di->path,pF->ST_fn,errno,strerror(errno));
      rc = StoreRC_SystemError ; 
      break ; 
    }
    iok++ ; 
  } while(0);
  if ( iok < 2 )
  {
    if ( iok > 0 )
      ism_common_free(ism_memory_store_misc,buff) ;
    return rc ; 
  }
  wH = (wHead_t *)buff ; 
  fd = pF->ST_fd ;
  for(;;)
  {
    ask = pInfo->di->block;  //assuming that pInfo->di->block > sizeof(wHead_t)
    bptr = buff ; 
    if ( (get = ism_store_persistIO(fd, bptr, ask, 1)) != ask )
    {
      if ( get != 0 ) // 0 means EOF which is ok here
      {
        TRACE(1,"Failed to read %lu bytes of persistent data from disk.  Quitting persistence recovery!!!\n",ask);
        rc = StoreRC_SystemError ; 
      }
      break ; 
    }
    if ( wH->cid != cid )
    {
      pReplay->bId = wH->cid ; 
      TRACE(5,"%s: cycleId missmatch: found %u, expected %u (nIo=%lu)\n",__FUNCTION__,wH->cid,cid,nIo);
      break ; 
    }
    if ( wH->hsz != sizeof(wHead_t) )
    {
      TRACE(3,"%s: header size missmatch: found %u, expected %lu (nIo=%lu)\n",__FUNCTION__,wH->hsz,sizeof(wHead_t),nIo);
      break ; 
    }
    if ( wH->tsz != sizeof(wTail_t) )
    {
      TRACE(3,"%s: tail size missmatch: found %u, expected %lu (nIo=%lu)\n",__FUNCTION__,wH->tsz,sizeof(wTail_t),nIo);
      break ; 
    }
    if ( wH->len > blen )
    {
      size_t l = wH->len + 4*pInfo->di->block ; 
      void *tmp = ism_common_realloc(ISM_MEM_PROBE(ism_memory_store_misc,18),buff,l) ; 
      if (!tmp)
      { 
        l = wH->len ; 
        tmp = ism_common_realloc(ISM_MEM_PROBE(ism_memory_store_misc,19),buff,l) ;
      }
      if (!tmp )
      {
        TRACE(1,"Failed to increase read buffer to %lu.  Quitting persistence recovery!!!\n",l);
        rc = StoreRC_AllocateError ;
        break ; 
      }
      bptr = buff = tmp ; 
      blen = l ; 
      wH = (wHead_t *)buff ; 
    }
    if ( wH->len != ask )
    {
      if ( wH->len > ask )
      {
        if ( ism_store_persistIO(fd, bptr+ask, wH->len-ask, 1) != wH->len-ask )
        {
          TRACE(1,"Failed to read %lu bytes of persistent data from disk.  Quitting persistence recovery!!!\n",wH->len-ask);
          rc = StoreRC_SystemError ; 
          break ; 
        }
      }
      else
      {
        TRACE(1,"Should not be here: write size (=%u) should have been at least block size (=%lu).  Quitting persistence recovery!!!\n",wH->len,ask);
        rc = StoreRC_SystemError ; 
        break ; 
      }
    }
    wT = (wTail_t *)(bptr + wH->len - wH->tsz) ; 
    bptr += wH->hsz ; 
    if ( wT->ts != wH->ts || wT->cid != wH->cid || wT->len != wH->len )
    {
      char tstr[128] ; 
      int k=0 ; 
      if ( wT->ts != wH->ts )
        k += snprintf(tstr+k,128-k,"wT->ts != wH->ts (%lu!=%lu), ",wT->ts,wH->ts) ; 
      if ( wT->cid != wH->cid )
        k += snprintf(tstr+k,128-k,"wT->cid != wH->cid (%u!=%u), ",wT->cid,wH->cid) ; 
      if ( wT->len != wH->len )
        k += snprintf(tstr+k,128-k,"wT->len != wH->len (%u!=%u), ",wT->len,wH->len) ; 
     #if 1
      TRACE(1,"Incomplete or inconsistent persistent data record read from disk (%s).  Assuming an interrupted write IO without commit, hence continue persistence recovery!!!\n",tstr);
     #else
      TRACE(1,"Incomplete or inconsistent persistent data read from disk (%s).  Quitting persistence recovery!!!\n",tstr);
      rc = StoreRC_SystemError ; 
     #endif
      break ; 
    }
    TRACE(9," Start replay record %lu of len %u having %u STs\n",nIo,wT->len,wH->nst);
    nIo++ ; 
    nSt += wH->nst ; 
    for ( i=0 ; i<wH->nst ; i++ )
    {
      int fl ;
      #if 0
      uint16_t msgType ; 
      memcpy(&fl,bptr,sizeof(fl)) ; 
      memcpy(&msgType, bptr+INT_SIZE, SHORT_SIZE) ; 
      if ( msgType == StoreHAMsg_UpdateActiveOid )
      {
        uint32_t opcount , rl;
        memcpy(&opcount, bptr+MSG_HEADER_SIZE-INT_SIZE, INT_SIZE) ; 
        rl = MSG_HEADER_SIZE + (opcount * (SHORT_SIZE+INT_SIZE+2*LONG_SIZE)) - INT_SIZE ; 
        if ( fl != rl )
        {
          fl = rl ; 
          memcpy(bptr,&fl,sizeof(fl)) ; 
        }
      }
      #endif
      if ( (rc = ism_store_persistReplayST(bptr)) != StoreRC_OK )
      {
        TRACE(1,"Failed to replay ST.  Quitting persistence recovery!!!\n");
        break ; 
      }
      memcpy(&fl,bptr,sizeof(fl)) ; 
      bptr += fl + pInfo->add2len ; 
      TRACE(9," After replying ST %u of len %u, offset %lu\n",i,fl,(bptr-buff));
    }
    if ( i<wH->nst )
      break ; 
  }

  TRACE(5,"%s: Finish playing %lu STs from %lu recs of file %s ; cI=%u, cJ=%u, cid=%u\n",__FUNCTION__,nSt,nIo,pF->ST_fn,cI,cJ,cid); 
  pReplay->nIo = nIo ; 
  close(fd) ; 
  ism_common_free(ism_memory_store_misc,buff) ; 
  return rc ; 
}


int ism_store_persistReplayST(char *pData)
{
   ismStore_memHAChannel_t *pHAChannel ;
   ismStore_memHAFragment_t *pFrag, frag;
   uint32_t fragLength ; 
   uint8_t flags, fFlowCtrlAck;
   char *ptr, *pInd;
   int chInd ; 
   int rc = StoreRC_OK;

   ptr = pData;
   memset(&frag, '\0', sizeof(frag));
   ismSTORE_getInt(ptr, fragLength);             // Fragment length
   pInd = ptr + pInfo->add2sid ; // SHORT + LONG + INT + CHAR
   ismSTORE_getInt(pInd, chInd);                  // "channel" index
   ptr += pInfo->add2ptr ; 

   if ( chInd >= pInfo->pChsSize )
   {
     size_t l ; 
     void *tmp ;
     l = chInd + 16 ; 
     tmp = ism_common_realloc(ISM_MEM_PROBE(ism_memory_store_misc,21),pInfo->pChs, l*sizeof(ismStore_memHAChannel_t *)) ; 
     if (!tmp )
     {
       l = chInd + 1 ; 
       tmp = ism_common_realloc(ISM_MEM_PROBE(ism_memory_store_misc,22),pInfo->pChs, l*sizeof(ismStore_memHAChannel_t *)) ; 
     }
     if (!tmp )
     {
       TRACE(1,"Failed to increase channel pointers array.  Quitting persistence recovery!!!\n");
       return StoreRC_AllocateError ;
     }
     pInfo->pChs = tmp ; 
     memset(pInfo->pChs+pInfo->pChsSize, 0, (l-pInfo->pChsSize)*sizeof(ismStore_memHAChannel_t *)) ; 
   }
   if (!pInfo->pChs[chInd] )
   {
     if (!(pInfo->pChs[chInd] = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,23),sizeof(ismStore_memHAChannel_t))) )
     {
       TRACE(1,"Failed to allocate memory for recovery channel.  Quitting persistence recovery!!!\n");
       return StoreRC_AllocateError ;
     }
     memset(pInfo->pChs[chInd], 0, sizeof(ismStore_memHAChannel_t)) ; 
     pInfo->pChs[chInd]->ChannelId = chInd ; 
   }
   pHAChannel = pInfo->pChs[chInd] ; 

   ismSTORE_getShort(ptr, frag.MsgType);         // Message type
   ismSTORE_getLong(ptr, frag.MsgSqn);           // Message sequence
   ismSTORE_getInt(ptr, frag.FragSqn);           // Fragment sequence
   ismSTORE_getChar(ptr, flags);                 // Fragment flags
   frag.flags = flags;
   fFlowCtrlAck = isFlowCtrlAck(flags);          // Flow control Ack
   ptr += INT_SIZE;                              // Reserved
   frag.pData = ptr;
   frag.DataLength = fragLength + INT_SIZE - (uint32_t)(ptr - pData);

   // Make sure that the fragments are received in order
   if (pHAChannel->pFragTail && (pHAChannel->pFragTail->MsgSqn != frag.MsgSqn || pHAChannel->pFragTail->FragSqn + 1 != frag.FragSqn))
   {
      TRACE(1, "Failed to read a persist fragment (ChannelId %d, FragLength %u, MsgType %u, MsgSqn %lu, FragSqn %u, IsLastFrag %d), because the header is not valid. ChannelMsgSqn %lu, ChannelFragSqn %u\n",
            pHAChannel->ChannelId, fragLength, frag.MsgType, frag.MsgSqn, frag.FragSqn, isLastFrag(frag.flags), pHAChannel->pFragTail->MsgSqn, pHAChannel->pFragTail->FragSqn);
      return StoreRC_SystemError;
   }

   TRACE(9, "A persist fragment (ChannelId %d, FragLength %u, MsgType %u, MsgSqn %lu, FragSqn %u, Flags 0x%x, IsLastFrag %d, FlowCtrlAck %u, DataLength %lu) has been read\n",
         pHAChannel->ChannelId, fragLength, frag.MsgType, frag.MsgSqn, frag.FragSqn, flags, isLastFrag(frag.flags), fFlowCtrlAck, frag.DataLength);

   if (isLastFrag(frag.flags))
   {
      // Add the last fragment at the end of the linked-list
      if (pHAChannel->pFragTail)
      {
         pHAChannel->pFragTail->pNext = &frag;
      }
      else
      {
         pHAChannel->pFrag = &frag;
      }
      pHAChannel->pFragTail = &frag;

      if (frag.MsgType == StoreHAMsg_StoreTran ||
          frag.MsgType == StoreHAMsg_UpdateActiveOid)
      {
         rc = ism_store_persistParseST(pHAChannel);
      }
      else
      if (frag.MsgType == StoreHAMsg_CreateGen ||
          frag.MsgType == StoreHAMsg_WriteGen ||
          frag.MsgType == StoreHAMsg_DeleteGen ||
          frag.MsgType == StoreHAMsg_ActivateGen ||
          frag.MsgType == StoreHAMsg_AssignRsrvPool)
      {
         rc = ism_store_persistParseGenST(pHAChannel);
      }
      else
      {
         TRACE(1,"%s: Should not be here!!! Internal error: MsgType=%u\n",__FUNCTION__,frag.MsgType);
         rc = StoreRC_SystemError ; 
      }

      // Release the memory of the fragments (except the last fragment which is located on the stack).
      while ((pFrag = pHAChannel->pFrag) != NULL)
      {
         if ((pHAChannel->pFrag = pFrag->pNext) != NULL)
         {
            ismSTORE_FREE(pFrag);
         }
      }
      pHAChannel->pFrag = pHAChannel->pFragTail = NULL;
   }
   else
   {
      if ((pFrag = (ismStore_memHAFragment_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,24),sizeof(ismStore_memHAFragment_t) + frag.DataLength)) == NULL)
      {
         TRACE(1, "Failed to allocate memory for persist fragment. ChannelId %d, FragLength %u, MsgType %u, MsgSqn %lu, FragSqn %u, IsLastFrag %d, DataLength %lu\n",
               pHAChannel->ChannelId, fragLength, frag.MsgType, frag.MsgSqn, frag.FragSqn, isLastFrag(frag.flags), frag.DataLength);
         return StoreRC_SystemError;
      }
      memcpy(pFrag, &frag, sizeof(ismStore_memHAFragment_t));
      pFrag->pData = (char *)pFrag + sizeof(*pFrag);
      memcpy(pFrag->pData, ptr, frag.DataLength);

      if (pHAChannel->pFragTail)
      {
         pHAChannel->pFragTail->pNext = pFrag;
      }
      else
      {
         pHAChannel->pFrag = pFrag;
      }
      pHAChannel->pFragTail = pFrag;
   }

   return rc;
}


int ism_store_persistParseGenST(ismStore_memHAChannel_t *pHAChannel)
{
   ismStore_memHAFragment_t *pFrag;
   ismStore_memHAMsgType_t msgType;
   ismStore_memOperationType_t opType;
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memGenHeader_t *pGenHeader;
   ismStore_memGeneration_t *pGen;
   ismStore_Handle_t offset;
   ismStore_GenId_t genId=0;
   uint8_t genIndex, poolId;
   uint32_t opcount, opLength;
   char *pData, *pBase ; 
   int rc = StoreRC_OK, i;

   pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress;
   pFrag = pHAChannel->pFrag;
   msgType = pFrag->MsgType;
   pData = pFrag->pData;

   ismSTORE_getInt(pData, opcount);           // Operations count
   TRACE(5, "Parsing a persist generation message. ChannelId %d, MsgType %u, MsgSqn %lu, " \
         "FragSqn %u, IsLastFrag %u, LastFrag %u, DataLength %lu, OpCount %u\n",
         pHAChannel->ChannelId, msgType, pFrag->MsgSqn, pFrag->FragSqn,
         isLastFrag(pFrag->flags), pHAChannel->pFragTail->FragSqn, pFrag->DataLength, opcount);

   ismSTORE_getShort(pData, opType);          // Operation type
   ismSTORE_getInt(pData, opLength);          // Operation data length
   if (opType != Operation_Null || opLength < SHORT_SIZE + 1)
   {
      TRACE(1, "Failed to parse a persist generation message. ChannelId %d, MsgType %u, MsgSqn %lu, FragSqn %u, " \
            "IsLastFrag %u, DataLength %lu, OpCount %u, OpType %d, OpDataLength %u\n",
            pHAChannel->ChannelId, pFrag->MsgType, pFrag->MsgSqn, pFrag->FragSqn, isLastFrag(pFrag->flags),
            pFrag->DataLength, opcount, opType, opLength);
      rc = StoreRC_SystemError;
      goto exit;
   }
   ismSTORE_getShort(pData, genId);           // Generation Id
   ismSTORE_getChar(pData, genIndex);         // Generation Index
   if (genIndex >= ismStore_memGlobal.InMemGensCount)
   {
      TRACE(1, "Failed to parse a persist fragment. ChannelId %d, MsgType %u, MsgSqn %lu, FragSqn %u, " \
            "IsLastFrag %u, DataLength %lu, OpCount %u, OpType %d, OpDataLength %u, GenId %u, GenIndex %u\n",
            pHAChannel->ChannelId, pFrag->MsgType, pFrag->MsgSqn, pFrag->FragSqn, isLastFrag(pFrag->flags),
            pFrag->DataLength, opcount, opType, opLength, genId, genIndex);
      rc = StoreRC_SystemError;
      goto exit;
   }
   opcount--;

   TRACE(5, "A persist generation message (GenId %u, Index %u) has been read. ChannelId %d, MsgType %u, MsgSqn %lu, FragSqn %u, IsLastFrag %u\n",
         genId, genIndex, pHAChannel->ChannelId, pFrag->MsgType, pFrag->MsgSqn, pFrag->FragSqn, isLastFrag(pFrag->flags));

   for(;;)
   {
      for (i=0; i < opcount; i++)
      {
         pBase = pData;
         ismSTORE_getShort(pData, opType);       // Operation type
         ismSTORE_getInt(pData, opLength);       // Operation data length

         if ( opType == Operation_CreateRecord )
         {
            ismSTORE_getLong(pData, offset);
            memcpy(ismStore_memGlobal.pStoreBaseAddress+offset, pData, opLength - LONG_SIZE);
            pData += (opLength - LONG_SIZE);
            if ( offset >= pMgmtHeader->MemSizeBytes )
            {
              ism_store_persistPrintGenHeader(ismStore_memGlobal.pStoreBaseAddress+offset,__LINE__);
              if ( offset != pMgmtHeader->InMemGenOffset[genIndex] )
              {
                 TRACE(1,"@@@ offset != pMgmtHeader->InMemGenOffset[genIndex] @@@ %lx %lx %u\n",offset,pMgmtHeader->InMemGenOffset[genIndex],genIndex);
              }
            }
            else
            if ( offset > 0 )
               ism_store_persistPrintGidChunk(ismStore_memGlobal.pStoreBaseAddress+offset,__LINE__);
            else
              ism_store_persistPrintMgmtHeader(ismStore_memGlobal.pStoreBaseAddress+offset,__LINE__);
         }
         else
         {
            TRACE(1, "Failed to parse a store operation in persist generation fragment, because the operation type %d is not valid. ChannelId %d, MsgType %u, MsgSqn %lu, FragSqn %u, IsLastFrag %u, DataLength %lu, OpCount %u, OpDataLength %u\n",
                  opType, pHAChannel->ChannelId, pFrag->MsgType, pFrag->MsgSqn, pFrag->FragSqn, isLastFrag(pFrag->flags), pFrag->DataLength, opcount, opLength);
            rc = StoreRC_SystemError;
            goto exit;
         }
         pData = pBase + (SHORT_SIZE + INT_SIZE + opLength);
      }

      if (!(pFrag = pFrag->pNext))
        break ; 

      pData = pFrag->pData;
      ismSTORE_getInt(pData, opcount);           // Operations count
      TRACE(9, "Parsing a persist fragment. ChannelId %d, MsgType %u, MsgSqn %lu, " \
            "FragSqn %u, IsLastFrag %u, DataLength %lu, OpCount %u\n",
            pHAChannel->ChannelId, pFrag->MsgType, pFrag->MsgSqn,
            pFrag->FragSqn, isLastFrag(pFrag->flags), pFrag->DataLength, opcount);
   }

   switch (msgType)
   {
      case StoreHAMsg_CreateGen:
         pGen = &ismStore_memGlobal.InMemGens[genIndex];
         pGenHeader = (ismStore_memGenHeader_t *)pGen->pBaseAddress;
         pGen->MaxRefsPerGranule = (pGenHeader->GranulePool[0].GranuleSizeBytes - sizeof(ismStore_memDescriptor_t) -
                                    offsetof(ismStore_memReferenceChunk_t, References)) / sizeof(ismStore_memReference_t);
         for (poolId=0; poolId < pGenHeader->PoolsCount; poolId++) {
            ism_store_memPreparePool(pGenHeader->GenId, pGen, &pGenHeader->GranulePool[poolId], poolId, 1);
         }
         TRACE(5, "A store generation (GenId %u, Index %u) has been created using persisted data. Version %lu, MemSizeBytes %lu, " \
               "GranuleSizeBytes %u, PoolsCount %u, DescriptorStructSize %u, SplitItemStructSize %u, RsrvPoolMemSizeBytes %lu, " \
               "pBaseAddress %p, State %u, Offset 0x%lx, MaxRefsPerGranule %u\n",
               pGenHeader->GenId, genIndex, pGenHeader->Version, pGenHeader->MemSizeBytes,
               pGenHeader->GranulePool[0].GranuleSizeBytes, pGenHeader->PoolsCount,
               pGenHeader->DescriptorStructSize, pGenHeader->SplitItemStructSize,
               pGenHeader->RsrvPoolMemSizeBytes, pGen->pBaseAddress,
               pGenHeader->State, pGen->Offset, pGen->MaxRefsPerGranule);
         break;

      case StoreHAMsg_WriteGen:
         pGen = &ismStore_memGlobal.InMemGens[1-genIndex];
         pGenHeader = (ismStore_memGenHeader_t *)pGen->pBaseAddress;
         if ((pGenHeader->StrucId == ismSTORE_MEM_GENHEADER_STRUCID) &&
             (pGenHeader->GenId != ismSTORE_RSRV_GEN_ID) &&
             (pGenHeader->State == ismSTORE_GEN_STATE_ACTIVE) &&
             (pGenHeader->PoolsCount > 0) )
         {
            for (poolId=0; poolId < pGenHeader->PoolsCount; poolId++) 
               ism_store_memPreparePool(pGenHeader->GenId, pGen, &pGenHeader->GranulePool[poolId], poolId, 1);
         }

       //pGenHeader->State = ismSTORE_GEN_STATE_CLOSE_PENDING;
         break;

      case StoreHAMsg_DeleteGen:
         TRACE(5, "Store generation (GenId %u) is being deleted from the disk\n", genId);
         break;
         
      case StoreHAMsg_ActivateGen:
         TRACE(5, "Store generation (GenId %u) is being activated\n", genId);
         break;

      case StoreHAMsg_AssignRsrvPool:
         TRACE(5, "The store management reserved pool as been assigned using persisted data.\n");
         break;

      default:
         TRACE(1, "Failed to parse a persist generation message (ChannelId %d, MsgSqn %lu), because the message type %d is not valid\n",
               pHAChannel->ChannelId, pHAChannel->pFrag->MsgSqn, msgType);
         rc = StoreRC_SystemError;
         goto exit;
   }

exit:

   return rc;
}


#define CHK_ADDR(x,y) if (!(x)) { TRACE(1,"ism_store_memMapHandleToAddress(%p) failed!!!\n",(void *)(y)); rc = StoreRC_prst_partialRecovery; break; }

int ism_store_persistParseST(ismStore_memHAChannel_t *pHAChannel)
{
   ismStore_memHAFragment_t *pFrag;
   ismStore_memOperationType_t opType;
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memGenHeader_t *pGenHeader;
   ismStore_memGeneration_t *pGen;
   ismStore_memDescriptor_t *pDescriptor, *pOwnerDescriptor;
   ismStore_memSplitItem_t *pSplit;
   ismStore_memReferenceChunk_t *pRefChunk;
   ismStore_memRefStateChunk_t *pRefStateChunk;
   ismStore_memReference_t *pRef;
   ismStore_memStateChunk_t *pStateChunk;
   ismStore_memState_t *pStateObject;
   ismStore_Handle_t handle, ownerHandle, chunkHandle, offset;
   ismStore_GenId_t genId=0;
   uint8_t *pRefState, fNewChunk;
   uint32_t opLength, opcount, mutexIndex;
   uint64_t baseOrderId;
   char *pData, *pBase;
   int rc = StoreRC_OK, i;

   pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress;

   TRACE(9, "Parsing a persist message. ChannelId %d, MsgType %u, MsgSqn %lu, LastFrag %u\n",
         pHAChannel->ChannelId, pHAChannel->pFrag->MsgType, pHAChannel->pFrag->MsgSqn, pHAChannel->pFragTail->FragSqn);

   for (pFrag = pHAChannel->pFrag; pFrag; pFrag = pFrag->pNext)
   {
      pData = pFrag->pData;
      ismSTORE_getInt(pData, opcount);           // Operations count

      TRACE(9, "Parsing a persist fragment. ChannelId %d, MsgType %u, MsgSqn %lu, " \
            "FragSqn %u, IsLastFrag %u, DataLength %lu, OpCount %u\n",
            pHAChannel->ChannelId, pFrag->MsgType, pFrag->MsgSqn,
            pFrag->FragSqn, isLastFrag(pFrag->flags), pFrag->DataLength, opcount);

      for (i=0; i < opcount; i++)
      {
         pBase = pData;
         ismSTORE_getShort(pData, opType);       // Operation type
         ismSTORE_getInt(pData, opLength);       // Operation data length

         switch (opType)
         {
            case Operation_CreateReference:
               ismSTORE_getLong(pData, handle);
               ismSTORE_getLong(pData, ownerHandle);
               ismSTORE_getLong(pData, baseOrderId);

               genId = ismSTORE_EXTRACT_GENID(handle);
               offset = ismSTORE_EXTRACT_OFFSET(handle);
               pGen = ism_store_memGetGenerationById(genId);
               pGenHeader = (ismStore_memGenHeader_t *)pGen->pBaseAddress;

               chunkHandle = ismSTORE_BUILD_HANDLE(genId, ismSTORE_ROUND(offset, pGenHeader->GranulePool[0].GranuleSizeBytes));
               pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(chunkHandle);
               CHK_ADDR(pDescriptor,chunkHandle);
               pRefChunk = (ismStore_memReferenceChunk_t *)(pDescriptor + 1);
               pOwnerDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(ownerHandle);
               pSplit = (ismStore_memSplitItem_t *)(pOwnerDescriptor + 1);

               // See whether we need to initialize the RefChunk
               fNewChunk = 0;
               mutexIndex = (ownerHandle / ismStore_memGlobal.MgmtSmallGranuleSizeBytes) % ismStore_memGlobal.RefCtxtLocksCount;
               pthread_mutex_lock(ismStore_memGlobal.pRefCtxtMutex[mutexIndex]);
               if ((pDescriptor->DataType & (~ismSTORE_DATATYPE_NOT_PRIMARY)) != ismSTORE_DATATYPE_REFERENCES ||
                   pRefChunk->OwnerHandle != ownerHandle ||
                   pRefChunk->BaseOrderId != baseOrderId ||
                   pRefChunk->OwnerVersion != pSplit->Version)
               {
                  pDescriptor->DataType = ismSTORE_DATATYPE_REFERENCES;
                  pDescriptor->Attribute = 0;
                  pDescriptor->State = 0;
                  pDescriptor->TotalLength = pDescriptor->DataLength = pGenHeader->GranulePool[0].GranuleDataSizeBytes;
                  pDescriptor->NextHandle = ismSTORE_NULL_HANDLE;

                  pRefChunk->OwnerHandle = ownerHandle;
                  pRefChunk->BaseOrderId = baseOrderId;
                  pRefChunk->ReferenceCount = pGen->MaxRefsPerGranule;
                  pRefChunk->OwnerVersion = pSplit->Version;
                  memset(pRefChunk->References, '\0', pRefChunk->ReferenceCount * sizeof(ismStore_memReference_t));
                  fNewChunk = 1;

                  TRACE(9, "A reference chunk (Handle 0x%lx) for owner (Handle 0x%lx, DataType 0x%x, Version %u) has been allocated on the Standby node. BaseOrderId %lu, RefHandle 0x%lx, MutexIndex %u\n",
                        chunkHandle, ownerHandle, pOwnerDescriptor->DataType, pSplit->Version, baseOrderId, handle, mutexIndex);
               }
               pthread_mutex_unlock(ismStore_memGlobal.pRefCtxtMutex[mutexIndex]);

               pRef = (ismStore_memReference_t *)ism_store_memMapHandleToAddress(handle);
               CHK_ADDR(pRef,handle);
               ismSTORE_getLong(pData, pRef->RefHandle);
               ismSTORE_getInt(pData, pRef->Value);
               ismSTORE_getChar(pData, pRef->State);

               break;

            case Operation_DeleteReference:
               ismSTORE_getLong(pData, handle);

               pRef = (ismStore_memReference_t *)ism_store_memMapHandleToAddress(handle);
               CHK_ADDR(pRef,handle);
               memset(pRef, '\0', sizeof(ismStore_memReference_t));
               break;

            case Operation_UpdateReference:
               ismSTORE_getLong(pData, handle);

               pRef = (ismStore_memReference_t *)ism_store_memMapHandleToAddress(handle);
               CHK_ADDR(pRef,handle);
               ismSTORE_getChar(pData, pRef->State);
               break;

            case Operation_CreateRecord:
               ismSTORE_getLong(pData, handle);
               pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(handle);
               if ( !pDescriptor )
               {
                 TRACE(1,"!!DBG: handle=%p, pDescriptor=%p, opLength=%u, pMgmtHeader=%p\n",(void *)handle,pDescriptor,opLength,pMgmtHeader);
                 fflush(stdout);
                 su_sleep(10,SU_MIL) ; 
               }
               CHK_ADDR(pRef,handle);
               memcpy(pDescriptor, pData, opLength - LONG_SIZE);
               pData += (opLength - LONG_SIZE);
               break;
            case Operation_DeleteRecord:
               ismSTORE_getLong(pData, handle);
               pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(handle);
               CHK_ADDR(pDescriptor,handle);

               pDescriptor->DataType = ismSTORE_DATATYPE_FREE_GRANULE;
               pDescriptor->NextHandle = ismSTORE_NULL_HANDLE;
               pDescriptor->DataLength = 0;
               pDescriptor->TotalLength = 0;
               pDescriptor->Attribute = 0;
               pDescriptor->State = 0;
               break;

            case Operation_UpdateRecord:
               ismSTORE_getLong(pData, handle);
               pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(handle);
               CHK_ADDR(pDescriptor,handle);

               ismSTORE_getLong(pData, pDescriptor->Attribute);
               ismSTORE_getLong(pData, pDescriptor->State);
               break;

            case Operation_UpdateRecordAttr:
               ismSTORE_getLong(pData, handle);
               pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(handle);
               CHK_ADDR(pDescriptor,handle);

               ismSTORE_getLong(pData, pDescriptor->Attribute);
               break;

            case Operation_UpdateRecordState:
               ismSTORE_getLong(pData, handle);
               pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(handle);
               CHK_ADDR(pDescriptor,handle);

               ismSTORE_getLong(pData, pDescriptor->State);
               break;

            case Operation_UpdateRefState:
               ismSTORE_getLong(pData, handle);
               ismSTORE_getLong(pData, ownerHandle);
               ismSTORE_getLong(pData, baseOrderId);

               genId = ismSTORE_EXTRACT_GENID(handle);
               offset = ismSTORE_EXTRACT_OFFSET(handle);
               chunkHandle = ismSTORE_BUILD_HANDLE(genId, ismSTORE_ROUND(offset, pMgmtHeader->GranulePool[ismSTORE_MGMT_SMALL_POOL_INDEX].GranuleSizeBytes));
               pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(chunkHandle);
               CHK_ADDR(pDescriptor,chunkHandle);
               pRefStateChunk = (ismStore_memRefStateChunk_t *)(pDescriptor + 1);
               pOwnerDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(ownerHandle);
               pSplit = (ismStore_memSplitItem_t *)(pOwnerDescriptor + 1);

               // See whether we need to initialize the RefStateChunk
               fNewChunk = 0;
               mutexIndex = (ownerHandle / ismStore_memGlobal.MgmtSmallGranuleSizeBytes) % ismStore_memGlobal.RefCtxtLocksCount;
               pthread_mutex_lock(ismStore_memGlobal.pRefCtxtMutex[mutexIndex]);
               if (pDescriptor->DataType != ismSTORE_DATATYPE_REFSTATES ||
                   pRefStateChunk->OwnerHandle != ownerHandle ||
                   pRefStateChunk->BaseOrderId != baseOrderId ||
                   pRefStateChunk->OwnerVersion != pSplit->Version)
               {
                  pDescriptor->DataType = ismSTORE_DATATYPE_REFSTATES;
                  pDescriptor->Attribute = 0;
                  pDescriptor->State = 0;
                  pDescriptor->TotalLength = pDescriptor->DataLength = pMgmtHeader->GranulePool[ismSTORE_MGMT_SMALL_POOL_INDEX].GranuleDataSizeBytes;
                  pDescriptor->NextHandle = ismSTORE_NULL_HANDLE;

                  pRefStateChunk->OwnerHandle = ownerHandle;
                  pRefStateChunk->BaseOrderId = baseOrderId;
                  pRefStateChunk->RefStateCount = ismStore_memGlobal.MgmtGen.MaxRefStatesPerGranule;
                  pRefStateChunk->OwnerVersion = pSplit->Version;
                  memset(pRefStateChunk->RefStates, ismSTORE_REFSTATE_NOT_VALID, pRefStateChunk->RefStateCount * sizeof(uint8_t));
                  fNewChunk = 1;

                  TRACE(9, "A refState chunk (Handle 0x%lx) for owner (Handle 0x%lx, DataType 0x%x, Version %u) " \
                        "has been allocated on the Standby node. BaseOrderId %lu, RefHandle 0x%lx, MutexIndex %u\n",
                        chunkHandle, ownerHandle, pOwnerDescriptor->DataType, pSplit->Version, baseOrderId, handle, mutexIndex);
               }
               pthread_mutex_unlock(ismStore_memGlobal.pRefCtxtMutex[mutexIndex]);

               pRefState = (uint8_t *)ism_store_memMapHandleToAddress(handle);
               CHK_ADDR(pRefState,handle);
               ismSTORE_getChar(pData, *pRefState);

               break;

            case Operation_CreateState:
               ismSTORE_getLong(pData, handle);
               ismSTORE_getLong(pData, ownerHandle);

               genId = ismSTORE_EXTRACT_GENID(handle);
               offset = ismSTORE_EXTRACT_OFFSET(handle);
               chunkHandle = ismSTORE_BUILD_HANDLE(genId, ismSTORE_ROUND(offset, ismStore_memGlobal.MgmtGranuleSizeBytes));
               pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(chunkHandle);
               CHK_ADDR(pDescriptor,chunkHandle);
               pStateChunk = (ismStore_memStateChunk_t *)(pDescriptor + 1);
               pOwnerDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(ownerHandle);
               pSplit = (ismStore_memSplitItem_t *)(pOwnerDescriptor + 1);

               // See whether we need to initialize the StateChunk
               fNewChunk = 0;
               mutexIndex = (ownerHandle / ismStore_memGlobal.MgmtSmallGranuleSizeBytes) % ismStore_memGlobal.StateCtxtLocksCount;
               pthread_mutex_lock(ismStore_memGlobal.pStateCtxtMutex[mutexIndex]);
               if (pDescriptor->DataType != ismSTORE_DATATYPE_STATES ||
                  pStateChunk->OwnerHandle != ownerHandle ||
                  pStateChunk->OwnerVersion != pSplit->Version)
               {
                  pDescriptor->DataType = ismSTORE_DATATYPE_STATES;
                  pDescriptor->Attribute = 0;
                  pDescriptor->State = 0;
                  pDescriptor->TotalLength = pDescriptor->DataLength = pMgmtHeader->GranulePool[ismSTORE_MGMT_POOL_INDEX].GranuleDataSizeBytes;
                  pDescriptor->NextHandle = ismSTORE_NULL_HANDLE;
                  pStateChunk->OwnerHandle = ownerHandle;
                  pStateChunk->OwnerVersion = pSplit->Version;
                  memset(pStateChunk->States, '\0', ismStore_memGlobal.MgmtGen.MaxStatesPerGranule * sizeof(ismStore_memState_t));
                  fNewChunk = 1;

                  TRACE(9, "A state chunk (Handle 0x%lx) for owner (Handle 0x%lx, DataType 0x%x, Version %u) " \
                        "has been allocated on the Standby node. StateHandle 0x%lx, MutexIndex %u\n",
                        chunkHandle, ownerHandle, pOwnerDescriptor->DataType, pSplit->Version, handle, mutexIndex);
               }
               pthread_mutex_unlock(ismStore_memGlobal.pStateCtxtMutex[mutexIndex]);

               pStateObject = (ismStore_memState_t *)ism_store_memMapHandleToAddress(handle);
               CHK_ADDR(pStateObject,handle);
               ismSTORE_getInt(pData, pStateObject->Value);
               ismSTORE_getChar(pData, pStateObject->Flag);

               TRACE(9, "A state object (Handle 0x%lx, Value %d, Flag %u, ChunkHandle 0x%lx, fNewChunk %u) for owner " \
                     "(Handle 0x%lx, DataType 0x%x, Version %u) has been added on the Standby node\n",
                     handle, pStateObject->Value, pStateObject->Flag, chunkHandle, fNewChunk,
                     ownerHandle, pOwnerDescriptor->DataType, pSplit->Version);

               break;

            case Operation_DeleteState:
               ismSTORE_getLong(pData, handle);
               pStateObject = (ismStore_memState_t *)ism_store_memMapHandleToAddress(handle);
               CHK_ADDR(pStateObject,handle);
               memset(pStateObject, '\0', sizeof(ismStore_memState_t));
               break;

            case Operation_UpdateActiveOid:
               ismSTORE_getLong(pData, handle);
               pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(handle);
               if (!pDescriptor )
               {
                 TRACE(1,"!!DBG: handle=%p, pDescriptor=%p, opLength=%u, pMgmtHeader=%p\n",(void *)handle,pDescriptor,opLength,pMgmtHeader);
                 fflush(stdout);
                 su_sleep(10,SU_MIL) ; 
               }
               CHK_ADDR(pDescriptor,handle);
               memcpy(pDescriptor, pData, opLength - LONG_SIZE);
               pData += (opLength - LONG_SIZE);
               break;

            default:
               TRACE(1, "Failed to parse a store operation in persist fragment, because the operation type %d is not valid. ChannelId %d, MsgType %u, MsgSqn %lu, FragSqn %u, IsLastFrag %u, DataLength %lu, OpCount %u, OpDataLength %u\n",
                     opType, pHAChannel->ChannelId, pFrag->MsgType, pFrag->MsgSqn, pFrag->FragSqn, isLastFrag(pFrag->flags), pFrag->DataLength, opcount, opLength);
               rc = StoreRC_SystemError;
               goto exit;
         }
         if ( rc == StoreRC_prst_partialRecovery ) goto exit;
         pData = pBase + (SHORT_SIZE + INT_SIZE + opLength);
      }
   }

exit:
   // No need to Send back an ACK

   return rc;
}

int ism_storePersist_prepareClean(void)
{
  int f , i,j;
  char *fn ; 
  int rc=StoreRC_OK ; 

  fn = pInfo->PState_fn ; 
  f = O_WRONLY | O_CREAT |O_CLOEXEC;
  do
  {
    if ( (pInfo->PState_fd = openat(pInfo->di->fdir,fn,f,ismStore_memGlobal.PersistedFileMode)) < 0 )
    {
      TRACE(1,"Failed to open file %s/%s for persistence, errno=%d (%s).\n",pInfo->di->path,fn,errno,strerror(errno));
      rc = StoreRC_SystemError ; 
      break ; 
    }
    memset(pInfo->PState,0,sizeof(persistState_t)) ; 
    pInfo->PState[0].cycleId = pInfo->PState[1].cycleId ;
    INC_CID(pInfo->PState[0].cycleId) ;
    pInfo->PState[0].recLen = sizeof(persistState_t) ;
    pInfo->PState[0].coldStart = 1 ;
    if ( ism_store_persistWritePState(__LINE__) < 0 )
    {
      TRACE(1,"Failed to write file %s/%s for persistence, errno=%d (%s).\n",pInfo->di->path,fn,errno,strerror(errno));
      rc = StoreRC_SystemError ; 
      break ; 
    }
    pInfo->PState[0].coldStart = 0 ;
    close(pInfo->PState_fd) ; 
    for ( j=0 ; j<2 && rc==StoreRC_OK ; j++ )
    {
      for ( i=0 ; i<2 && rc==StoreRC_OK; i++ )
      {
        persistFiles_t *pF = &pInfo->pFiles[i][j] ; 
        unlinkat(pInfo->di->fdir, pF->CPM_fn, 0) ;
        unlinkat(pInfo->di->fdir, pF->CPG_fn, 0) ;
      }
    }
  } while(0);
  return rc ; 
}

int ism_storePersist_emptyDir(const char *path)
{
  int rc;
  ismStoe_DirInfo di[1];
  struct dirent *de ; 

  if ((rc = ism_storeDisk_initDir(path, di)) != StoreRC_OK )
    return rc ; 

  rewinddir(di->pdir);
  while ( (de = readdir(di->pdir)) )
  {
   #ifdef _DIRENT_HAVE_D_TYPE
    if ( de->d_type && de->d_type != DT_REG ) continue ; 
   #endif
    if (!memcmp(de->d_name,"ST_",3) || !memcmp(de->d_name,"CPM_",4) || !memcmp(de->d_name,"CPG_",4) || !memcmp(de->d_name,"PState",6) )
    {
      if ( unlinkat(di->fdir, de->d_name,0) )
      {
        rc = StoreRC_SystemError ; 
        break ; 
      }
    }
  }
  ism_common_free(ism_memory_store_misc,di->path);
  closedir(di->pdir) ; 
  return rc ; 
}


int ism_storePersist_havePState(void)
{
  return pInfo->PState[0].recLen ; 
}

int ism_storePersist_coldStart(void)
{
  return pInfo->PState[0].coldStart ; 
}

int ism_storePersist_createCP(int needCP)
{
  int rc=StoreRC_OK ; 
  int f ; 
  char *fn ; 
  ismStore_memMgmtHeader_t *pMgmtHeader;
  ismStore_DiskTaskParams_t *dtp ;
  ismStore_DiskBufferParams_t *bp ;

  pthread_mutex_lock(pInfo->lock) ; 
  while ( pInfo->waitCP && !pInfo->goDown )
  {
    pthread_mutex_unlock(pInfo->lock) ; 
    su_sleep(100,SU_MIL) ; 
    pthread_mutex_lock(pInfo->lock) ; 
  }
  if ( pInfo->goDown )
    rc = StoreRC_SystemError ; 
  else
    pInfo->waitCP = 1 ; 
  pthread_mutex_unlock(pInfo->lock) ; 
  if ( rc != StoreRC_OK )
    goto exit ;
    
  TRACE(5,"%s: needCP=%u (%d), cid=%u %u, curI=%u, curJ=%u\n",__FUNCTION__,pInfo->needCP,needCP,pInfo->PState[0].cycleId, pInfo->PState[1].cycleId,pInfo->curI,pInfo->curJ);
  if (!pInfo->needCP && !needCP)
    goto exit ; 

  pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress;
  if ( pInfo->curI != pMgmtHeader->ActiveGenIndex )
  {
    TRACE(1,"%s: @@@ pInfo->curI (%u) != pMgmtHeader->ActiveGenIndex (%u) \n",__FUNCTION__,pInfo->curI,pMgmtHeader->ActiveGenIndex);
  }
  pInfo->curI = pMgmtHeader->ActiveGenIndex ; 
  pInfo->curJ = 1 - pInfo->PState[0].startFile[pInfo->curI] ; 
  if ( (rc = ism_storePersist_prepareCP(pInfo->curI,pInfo->curJ)) != StoreRC_OK )
  {
    TRACE(1,"%s: ism_storePersist_prepareCP failed, rc=%d\n",__FUNCTION__,rc);
    pInfo->needCP = 1 ;
    goto exit ;
  }
  dtp = pInfo->DiskTaskParams_CPM ; 
  bp = dtp->BufferParams ; 
  ism_storePersist_calcStopToken(bp->pBuffer, bp->BufferLength, pInfo->PState[0].stopToken, sizeof(pInfo->PState[0].stopToken)) ; 
  memcpy(pMgmtHeader->PersistToken,pInfo->PState[0].stopToken, sizeof(pInfo->PState[0].stopToken)) ; 
  if ( (rc = ism_storePersist_writeCP()) != StoreRC_OK )
  {
    TRACE(1,"%s: ism_storePersist_writeCP failed, rc=%d\n",__FUNCTION__,rc);
    pInfo->needCP = 1 ;
    goto exit ;
  }

  pthread_mutex_lock(pInfo->lock) ; 
  for(;;)
  {
    if ( pInfo->fileWriteState[0] == 1 && pInfo->fileWriteState[1] == 1 )
      break ; 
    if ( pInfo->fileWriteState[0] ==-1 || pInfo->fileWriteState[1] ==-1 || pInfo->goDown )
    {
      TRACE(1,"%s: pInfo->fileWriteState[0] (%d) ==-1 || pInfo->fileWriteState[1] (%d) ==-1 || pInfo->goDown (%d)\n",__FUNCTION__,
            pInfo->fileWriteState[0], pInfo->fileWriteState[1], pInfo->goDown);
      rc = StoreRC_SystemError ; 
      break ; 
    }
    pthread_cond_wait(pInfo->cond, pInfo->lock);
  }
  pthread_mutex_unlock(pInfo->lock) ; 
  if ( rc != StoreRC_OK )
    goto exit ;
  fn = pInfo->PState_fn ; 
  f = O_WRONLY | O_CREAT |O_CLOEXEC;
  do
  {
    if ( (pInfo->PState_fd = openat(pInfo->di->fdir,fn,f,ismStore_memGlobal.PersistedFileMode)) < 0 )
    {
      TRACE(1,"Failed to open file %s/%s for persistence, errno=%d (%s).\n",pInfo->di->path,fn,errno,strerror(errno));
      rc = StoreRC_SystemError ; 
      break ; 
    }
    if (!pInfo->recoveryDone)
    {
      int i;
      for ( i=0 ; i<4 ; i++ )
      {
        INC_CID(pInfo->PState[1].cycleId) ; 
      }
    }
    pInfo->PState[0].cycleId = pInfo->PState[1].cycleId ;
    INC_CID(pInfo->PState[0].cycleId) ; 
    pInfo->PState[0].recLen = sizeof(persistState_t) ;
    pInfo->PState[0].startGen = pInfo->curI ; 
    pInfo->PState[0].genTr = 0 ; 
    pInfo->PState[0].startFile[0] = pInfo->curJ ; 
    pInfo->PState[0].startFile[1] = pInfo->curJ ; 
    pInfo->PState[0].coldStart = 0 ; 
    pInfo->PState[0].isStandby = 0 ; 
    memcpy(&pInfo->PState[1],&pInfo->PState[0],sizeof(persistState_t)) ; 
    if ( ism_store_persistWritePState(__LINE__) < 0 )
    {
      TRACE(1,"Failed to write file %s/%s for persistence, errno=%d (%s).\n",pInfo->di->path,fn,errno,strerror(errno));
      rc = StoreRC_SystemError ; 
    }
    close(pInfo->PState_fd) ; 
    pInfo->needCP = needCP ; 
  } while(0);

  if (rc != StoreRC_OK)
    pInfo->needCP = 1 ;

 exit:
  pthread_mutex_lock(pInfo->lock) ; 
  pInfo->waitCP = 0 ; 
  pthread_mutex_unlock(pInfo->lock) ; 

  return rc ; 
}

static int32_t ism_storePersist_zeroFile(int fd, size_t size)
{

   if (!fallocate(fd,0,0,size) ) return ISMRC_OK;
   TRACE(5,"%s: fallocate failed, errno=%d (%s) ; Skipping zeroing the file...\n",__FUNCTION__,errno,strerror(errno));
   return ISMRC_OK;
#if 0
   double tt,nt;
   char *pBuff;
   size_t offset, blockSizeBytes, bytesWritten;
   int32_t rc = ISMRC_OK;

   blockSizeBytes = fpathconf(fd, _PC_REC_XFER_ALIGN);
   if (posix_memalign((void **)&pBuff, blockSizeBytes, blockSizeBytes))
   {
      TRACE(1, "Failed to allocate memory for validating the shared-memory available space. BlockSizeBytes %lu, TotalMmemSizeBytes %lu.\n",
            blockSizeBytes, size);
      rc = ISMRC_AllocateError;
      return rc;
   }
   memset(pBuff, '\0', blockSizeBytes);
  tt = su_sysTime();
  nt = tt + 1e1;
   for (offset=0; offset < size; offset += blockSizeBytes)
   {
      if ((bytesWritten = write(fd, pBuff, blockSizeBytes)) != blockSizeBytes)
      {
         TRACE(1, "Failed to initialize the store, because there is not enough available memory for the shared-memory object. TotalMmemSizeBytes %lu, BlockSizeBytes %lu, Offset %lu, errno %d.\n",
               size, blockSizeBytes, offset, errno);
         rc = ISMRC_StoreNotAvailable;
         break;
      }
      if ( nt < su_sysTime() )
      {
        TRACE(5,"%s: offset is %lu After %f seconds...\n",__FUNCTION__,offset,nt-tt);
        nt += 1e1 ; 
      }
   }
  tt = su_sysTime()-tt;
  TRACE(5,"%s: %f secs to sero the file: fd=%d , size=%lu, block=%lu\n",__FUNCTION__,tt,fd,size,blockSizeBytes);

   lseek(fd, 0, SEEK_SET);
   ismSTORE_FREE(pBuff);

   return rc;
#endif
}


int ism_storePersist_start(void)
{
  int rc=StoreRC_OK ; 
  int i,j,iok=0, oki ; 

  do
  {
    int f ; 
    char *fn ; 
    fn = pInfo->PState_fn ; 
    f = O_WRONLY | O_CREAT |O_CLOEXEC;
    if ( (pInfo->PState_fd = openat(pInfo->di->fdir,fn,f,ismStore_memGlobal.PersistedFileMode)) < 0 )
    {
      TRACE(1,"Failed to open file %s/%s for persistence, errno=%d (%s).\n",pInfo->di->path,fn,errno,strerror(errno));
      rc = StoreRC_SystemError ; 
      break ; 
    }
    memcpy(&pInfo->PState[1],&pInfo->PState[0],sizeof(persistState_t)) ; 
    iok++;
    if ( (rc = ism_storePersist_openSTfiles(4)) != StoreRC_OK )
            break ; 
    iok++ ; 
    if ( rc != StoreRC_OK )
      break ; 
    do
    {
      char *th_nm, nm[32];
      int nTh=0 ; 
      oki = 0  ; 
      th_nm = "shmPersist" ; 
      if ( ism_common_startThread(&pInfo->tid[nTh++], ism_store_persistThread, pInfo, NULL, 0, ISM_TUSAGE_NORMAL,0, th_nm, "Persist_Shm_Store_to_Disk") )
      {
        rc = StoreRC_SystemError ; 
        break ; 
      }
      if ( pInfo->useSigTh )
      {
        th_nm = "signalPersist" ; 
        if ( ism_common_startThread(&pInfo->tid[nTh++], ism_store_persistSigThread, pInfo, NULL, 0, ISM_TUSAGE_NORMAL,0, th_nm, "Persist_Sig_Store_to_Disk") )
        {
          rc = StoreRC_SystemError ; 
          break ; 
        }
      }
      if ( pInfo->enableHA )
      {
        th_nm = "haPersistSend" ; 
        for ( i=0 ; i<pInfo->numTHtx ; i++ )
        {
          snprintf(nm,32,"haPersistTx.%d",i);
          if ( ism_common_startThread(&pInfo->tid[nTh++], ism_store_persistHaSThread, pInfo, NULL, i, ISM_TUSAGE_NORMAL,0, nm, "Persist_HA_Send_to_SB") )
          {
            rc = StoreRC_SystemError ; 
            break ; 
          }
        }
        if ( rc != StoreRC_OK )
          break ; 
        th_nm = "haPersistRx" ; 
        if ( ism_common_startThread(&pInfo->tid[nTh++], ism_store_persistHaRThread, pInfo, NULL, 0, ISM_TUSAGE_NORMAL,0, th_nm, "Persist_HA_Recv_fr_SB") )
        {
          rc = StoreRC_SystemError ; 
          break ; 
        }
      }
      th_nm = "asyncPersist" ; 
      for ( i=0 ; i<pInfo->numTHrx ; i++ )
      {
        snprintf(nm,32,"asyncPersist.%d",i);
        if ( ism_common_startThread(&pInfo->tid[nTh++], ism_store_persistAsyncThread, pInfo, NULL, i, ISM_TUSAGE_NORMAL,0, nm, "Persist_deliver_Async_cb") )
        {
          rc = StoreRC_SystemError ; 
          break ; 
        }
      }
      if ( rc != StoreRC_OK )
        break ; 
      pthread_mutex_lock(pInfo->lock) ; 
      while ( pInfo->thUp < nTh )
      {
        pthread_mutex_unlock(pInfo->lock) ; 
        su_sleep(1,SU_MIC) ; 
        pthread_mutex_lock(pInfo->lock) ; 
      }
      pthread_mutex_unlock(pInfo->lock) ; 
      oki++ ; 
    } while(0) ; 
    if ( oki < 1 )
    {
      break ; 
    }
    iok++ ; 
  } while(0) ; 
  if ( iok < 3 )
  {
    if ( iok > 0 )
    {
      close(pInfo->PState_fd ) ; 
    }
    if ( iok > 1 )
    {
      for ( j=0 ; j<2 && rc==StoreRC_OK ; j++ )
      {
        for ( i=0 ; i<2 && rc==StoreRC_OK; i++ )
        {
          persistFiles_t *pF = &pInfo->pFiles[i][j] ; 
          ismSTORE_CLOSE_FD(pF->ST_fd) ; 
        }
      }
    }
  }
  else
  {
   // INC_CID(pInfo->PState[1].cycleId) ;
    pthread_mutex_lock(pInfo->lock) ; 
    pInfo->init=PERSIST_STATE_STARTED;
    pthread_mutex_unlock(pInfo->lock) ; 
  }
  return rc ; 
}

static int ism_store_persistWaitForSpace(ismStore_memStream_t  *pStream)
{
  int rc;
  ismStore_persistInfo_t *pPersist = pStream->pPersist ; 
  while ((pPersist->BuffLen + pPersist->Buf0Len > pPersist->BuffSize ||
          pPersist->NumCBs >= PERSIST_MAX_CBs) && !pInfo->goDown )
  {
    pStream->pPersist->Waiting++;
    ismSTORE_COND_WAIT(&pStream->Cond, &pStream->Mutex);
    pStream->pPersist->Waiting--;
  }
  if ( pInfo->goDown )
  {
    if (ismStore_memGlobal.State == ismSTORE_STATE_DISKERROR)
      rc = ISMRC_StoreDiskError ; 
    else
    if (ismStore_memGlobal.State == ismSTORE_STATE_ALLOCERROR)
      rc = ISMRC_StoreAllocateError ; 
    else
      rc = ISMRC_StoreNotAvailable ; 
  }
  else
    rc = ISMRC_OK ; 
  return rc ; 
}

int ism_storePersist_openStream(ismStore_memStream_t *pStream)
{
  pthread_mutex_lock(pInfo->lock) ; 
  if ( pStream->fHighPerf )
  {
    if ( pStream->fHighPerf >= pInfo->numTHrx )
      pStream->pPersist->indRx = pStream->fHighPerf % pInfo->numTHrx ; 
    else
      pStream->pPersist->indRx = pInfo->indH % pInfo->numTHrx ; 
    pStream->pPersist->indTx = pInfo->indH % pInfo->numTHtx ; 
    pInfo->indH++ ; 
  }
  else
  {
    pStream->pPersist->indRx = pInfo->numTHrx - (pInfo->indL % pInfo->numTHrx) - 1 ; 
    pStream->pPersist->indTx = pInfo->numTHtx - (pInfo->indL % pInfo->numTHtx) - 1 ; 
    pInfo->indL++ ; 
  }
  pthread_mutex_unlock(pInfo->lock) ; 
  TRACE(5," Stream %u with fHighPerf %u was assigned to asynThread %u and to haTx thread %u\n",
           pStream->hStream,pStream->fHighPerf,pStream->pPersist->indRx,pStream->pPersist->indTx);
  return ISMRC_OK ;
}

// must be called while holding the stream mutex
int ism_storePersist_closeStream(ismStore_memStream_t *pStream)
{
  if ( pInfo->goDown ) return ISMRC_OK ;
  return ism_storePersist_completeST_(pStream);
#if 0
  while ( (pStream->pPersist->State & PERSIST_STATE_PENDING) && !pInfo->goDown )
  {
    pStream->pPersist->Waiting++;
    ismSTORE_COND_WAIT(&pStream->Cond, &pStream->Mutex);
    pStream->pPersist->Waiting--;
  }
  if ( pInfo->goDown )
  {
    if (ismStore_memGlobal.State == ismSTORE_STATE_DISKERROR)
      return ISMRC_StoreDiskError ; 
    if (ismStore_memGlobal.State == ismSTORE_STATE_ALLOCERROR)
      return ISMRC_StoreAllocateError ; 
    return ISMRC_StoreNotAvailable ; 
  }
  return ISMRC_OK ;
#endif 
}

int ism_storePersist_completeST_(ismStore_memStream_t  *pStream)
{
  int n=0, rc;
 #if USE_STATS
  pStream->pPersist->numComplete++;
 #endif
  if ( PERSIST_COMPLETED(pStream) )
    return ISMRC_OK ; 
  if ( pInfo->useYield )
  {
    ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);
  }
  ism_common_backHome();
  for (;;)
  {
    if ( PERSIST_COMPLETED(pStream) )
    {
      rc = ISMRC_OK ; 
      break ; 
    }
    if ( pInfo->goDown )
    {
      if (ismStore_memGlobal.State == ismSTORE_STATE_DISKERROR)
        rc = ISMRC_StoreDiskError ; 
      else
      if (ismStore_memGlobal.State == ismSTORE_STATE_ALLOCERROR)
        rc = ISMRC_StoreAllocateError ; 
      else
        rc = ISMRC_StoreNotAvailable ; 
      break ; 
    }
    if ( pInfo->useYield )
    {
      sched_yield();
      if ( ++n > 100 )
      {
        ismSTORE_MUTEX_LOCK(&pStream->Mutex);
        ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);
        n = 0 ; 
      }
    }
    else
    {
      pStream->pPersist->Waiting++;
      ismSTORE_COND_WAIT(&pStream->Cond, &pStream->Mutex);
      pStream->pPersist->Waiting--;
    }
  }
  ism_common_going2work();
  if ( pInfo->useYield )
  {
    ismSTORE_MUTEX_LOCK(&pStream->Mutex);
  }
  return rc ; 
}
int ism_storePersist_completeST(ismStore_memStream_t  *pStream)
{
  int rc;
  ismSTORE_MUTEX_LOCK(&pStream->Mutex);
  rc = ism_storePersist_completeST_(pStream);
  ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);
  return rc;
}


typedef struct
{
  char   *pBuffer ; 
  char   *pPos ; 
  char   *pUpTo ; 
} persistBuff_t ; 

static int ism_storePersist_getBuff(ismStore_memStream_t  *pStream, persistBuff_t *pB,
                                    uint32_t requiredLength, ismStore_memHAMsgType_t msgType, uint32_t *opcount, uint32_t  block)
{
   ismStore_persistInfo_t *pPersist = pStream->pPersist;
   int rc=StoreRC_OK;

   if (!pB->pBuffer)
   {
     pPersist->FragSqn = 0;
     goto alloc ; 
   }

 //if (pB->pBuffer)
   {
      char *ptr;
      uint8_t flags=0;
      void *pFlags;
      void *jumpto ; 
      if (requiredLength)
      {
        if ( pB->pPos + requiredLength + 64 <= pB->pUpTo ) 
          return rc ; 
        block = 1 ;
       #if USE_STATS
        pPersist->numFullBuff++ ; 
       #endif
        jumpto = &&alloc ; 
      }
      else
      {
        setLastFrag(flags) ;            // Last fragment
        setNoAck(   flags) ;            // No Ack
        jumpto = &&eof ; 
      }
      // Set the message fragment header
      ptr = pB->pBuffer;
      ismSTORE_putInt(ptr, (pB->pPos-pB->pBuffer-INT_SIZE)) // Fragment length
      ismSTORE_putShort(ptr, msgType);                      // Message type
      ismSTORE_putLong(ptr, pPersist->MsgSqn);              // Message sequence
      ismSTORE_putInt(ptr, pPersist->FragSqn);              // Fragment sequence
      pFlags = (uint8_t *)ptr ;                             
      ismSTORE_putChar(ptr, flags);                         // Flags
      ismSTORE_putInt(ptr, pStream->hStream);               // Stream Id uses the Reserved field
   // ismSTORE_putInt(ptr, 0);                              // Reserved
      ismSTORE_putInt(ptr, *opcount);                       // Operations count

      ismSTORE_MUTEX_LOCK(&pStream->Mutex);
      pPersist->Buf0Len  = (pB->pPos-pB->pBuffer);
     #if USE_HISTO
      double ct = ism_common_readTSC() ; 
     #endif
      if ( pPersist->BuffLen + pPersist->Buf0Len > pPersist->BuffSize )
      {
        #if USE_STATS
         pPersist->numFullBuff++ ; 
        #endif
        if ( (rc = ism_store_persistWaitForSpace(pStream)) != ISMRC_OK )
        {
           ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);
           return rc ; 
        }
      }
      if (!pPersist->BuffLen )
      {
        pPersist->BuffLen = pPersist->Buf0Len ; 
        pPersist->Buf0Len = 0 ; 
        pPersist->Buf0    = pPersist->Buff    ; 
        pPersist->Buff    = pB->pBuffer ; 
       #if USE_HISTO
        if (!pPersist->ct )
          pPersist->ct = ct ; 
       #endif
      }
      else
      {
        pFlags = pPersist->Buff + pPersist->BuffLen + (pFlags - pPersist->Buf0) ; 
        memcpy(pPersist->Buff + pPersist->BuffLen, pPersist->Buf0, pPersist->Buf0Len) ; 
        pPersist->BuffLen += pPersist->Buf0Len ;
        pPersist->Buf0Len = 0 ; 
      }
      if ( !requiredLength )
      {
        pPersist->lastSTflags = pFlags ; 
        if ( pPersist->curCB->pCallback )
        {
          if ( pPersist->NumCBs >= PERSIST_MAX_CBs )
          {
           #if USE_STATS
            pPersist->numFullCB++ ; 
           #endif
            if ( (rc = ism_store_persistWaitForSpace(pStream)) != ISMRC_OK )
            {
               ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);
               return rc ; 
            }
          }
          pPersist->curCB->MsgSqn = pPersist->MsgSqn ; 
          pPersist->curCB->TimeStamp = ism_common_readTSC() ;
          TRACE(9,"%s: pPersist->curCB->TimeStamp = %g for stream %u\n",__FUNCTION__,pPersist->curCB->TimeStamp,pStream->hStream);
         #if USE_HISTO
          {
            double *cts ; 
            int ind = pPersist->MsgSqn & (CB_ARRAY_SIZE-1) ; 
            double ct = ism_common_readTSC() ; 
            pPersist->curCB->deliveryTime = ct + deliveryDelay ; 
            if ( !pPersist->RSRV )
            {
              pPersist->RSRV = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,26),CB_ARRAY_SIZE*sizeof(double)) ; 
            }
            cts = (double *)pPersist->RSRV ; 
            if ( cts && !cts[ind] )
              cts[ind] = ct ; 
          }
         #else
          if ( deliveryDelay )
            pPersist->curCB->deliveryTime = ism_common_readTSC() + deliveryDelay ; 
         #endif
          pPersist->CBs[pPersist->NumCBs++] = pPersist->curCB[0] ; 
        }
       #if USE_STATS
        else
        if ( pPersist->curCB->numCBs )
        {
          pPersist->numCommit++ ; 
        }
       #endif
        pPersist->MsgSqn++ ; 
      }
      pStream->pPersist->State |= PERSIST_STATE_PENDING ;
      pInfo->go2Work = 1;
      pthread_cond_signal(pInfo->cond);

      pPersist->FragsCount++;
      pPersist->FragSqn++;
      pPersist->NumSTs++ ; 
      if ( block )
        rc = ism_storePersist_completeST_(pStream);
      ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);
      pB->pBuffer = NULL;
      goto *jumpto ; 
   }
// else
// {
//    pPersist->FragSqn = 0;
// }

   // Allocate a new buffer
  alloc:
   {
      pB->pBuffer = pPersist->Buf0 + pPersist->Buf0Len; 
      pB->pPos  = pB->pBuffer + MSG_HEADER_SIZE;      // Skip on the message header
      pB->pUpTo = pPersist->Buf0 + pPersist->BuffSize ;
      *opcount = 0;
   }
  eof:
   return rc;
}

static int ism_storePersist_writeST_(ismStore_memStream_t *pStream,int block)
{
   ismStore_memDescriptor_t *pDescriptor, *pOpDescriptor;
   ismStore_memStoreTransaction_t *pTran;
   ismStore_memStoreOperation_t *pOp;
   ismStore_memSplitItem_t *pSplit;
   ismStore_memReferenceChunk_t *pRefChunk;
   ismStore_memRefStateChunk_t *pRefStateChunk;
   ismStore_memStateChunk_t *pStateChunk;
   ismStore_memState_t *pStateObject;
   ismStore_Handle_t handle, offset, chunkOff;
   ismStore_GenId_t genId;
   ismStore_memHAMsgType_t msgType = StoreHAMsg_StoreTran;
   persistBuff_t pB={0};
   char *pTmp, *pLen, *tranBase, *genBase;
   int loop, rc, i;
   uint32_t opcount, reqLen;
   ismStore_Handle_t hStoreTran = pStream->hStoreTranHead ; 

   genId = ismSTORE_EXTRACT_GENID(hStoreTran);
   tranBase = ism_store_getBaseAddr(genId);
   pDescriptor = tranBase ? (ismStore_memDescriptor_t *)(tranBase + ismSTORE_EXTRACT_OFFSET(hStoreTran)) : NULL;
   while (pDescriptor)
   {
      pTran = (ismStore_memStoreTransaction_t *)(pDescriptor + 1);
      if (pTran->OperationCount == 0) { break; }

      if (pTran->State != ismSTORE_MEM_STORETRANSACTIONSTATE_ACTIVE)
      {
         TRACE(1, "Failed to send the store-transaction to the standby node, because the ST state (%d) is not valid\n", pTran->State);
         return StoreRC_SystemError;
      }

      for (i=0, pOp=pTran->Operations; i < pTran->OperationCount; i++, pOp++)
      {
         switch (pOp->OperationType)
         {
            case Operation_CreateReference:

               if ((rc = ism_storePersist_getBuff(pStream, &pB, 64, msgType, &opcount, block)) != StoreRC_OK) return rc;
               handle = pOp->CreateReference.Handle;
               genId = ismSTORE_EXTRACT_GENID(handle);
               offset = ismSTORE_EXTRACT_OFFSET(handle);
               chunkOff = ismSTORE_ROUND(offset, ismStore_memGlobal.pGensMap[genId]->GranulesMap[0].GranuleSizeBytes);
               genBase = ism_store_getBaseAddr(genId) ; 
               pOpDescriptor = (ismStore_memDescriptor_t *)(genBase + chunkOff);
               pRefChunk = (ismStore_memReferenceChunk_t *)(pOpDescriptor + 1);

               ismSTORE_putShort(pB.pPos, pOp->OperationType);
               pLen = pB.pPos; pB.pPos += INT_SIZE; 
               ismSTORE_putLong(pB.pPos, handle);
               ismSTORE_putLong(pB.pPos, pRefChunk->OwnerHandle);
               ismSTORE_putLong(pB.pPos, pRefChunk->BaseOrderId);
               ismSTORE_putLong(pB.pPos, pOp->CreateReference.RefHandle);
               ismSTORE_putInt(pB.pPos, pOp->CreateReference.Value);
               ismSTORE_putChar(pB.pPos, pOp->CreateReference.State);
               ismSTORE_putInt(pLen, pB.pPos - pLen - INT_SIZE);
               opcount++;
               break;

            case Operation_UpdateReference:

               if ((rc = ism_storePersist_getBuff(pStream, &pB, 64, msgType, &opcount, block)) != StoreRC_OK) return rc;
               handle = pOp->UpdateReference.Handle;
               ismSTORE_putShort(pB.pPos, pOp->OperationType);
               pLen = pB.pPos; pB.pPos += INT_SIZE; 
               ismSTORE_putLong(pB.pPos, handle);
               ismSTORE_putChar(pB.pPos, pOp->UpdateReference.State);
               ismSTORE_putInt(pLen, pB.pPos - pLen - INT_SIZE);
               opcount++;
               break;

            case Operation_DeleteReference:

               if ((rc = ism_storePersist_getBuff(pStream, &pB, 64, msgType, &opcount, block)) != StoreRC_OK) return rc;
               handle = pOp->DeleteReference.Handle;
               ismSTORE_putShort(pB.pPos, pOp->OperationType);
               pLen = pB.pPos; pB.pPos += INT_SIZE; 
               ismSTORE_putLong(pB.pPos, handle);
               ismSTORE_putInt(pLen, pB.pPos - pLen - INT_SIZE);
               opcount++;
               break;

            case Operation_CreateRecord:
               handle = pOp->CreateRecord.Handle;
               genId = ismSTORE_EXTRACT_GENID(handle);
               genBase = ism_store_getBaseAddr(genId) ; 
               loop=0;
               for(;;)
               {
                  if (handle == ismSTORE_NULL_HANDLE)
                  {
                    if ( pOp->CreateRecord.LDHandle && !loop )
                    {
                       handle = pOp->CreateRecord.LDHandle;
                       genId = ismSTORE_EXTRACT_GENID(handle);
                       genBase = ism_store_getBaseAddr(genId) ; 
                       loop++;
                     }
                     else
                       break;
                  }

                  pOpDescriptor = (ismStore_memDescriptor_t *)(genBase + ismSTORE_EXTRACT_OFFSET(handle));
                  reqLen = pOpDescriptor->DataLength + sizeof(*pOpDescriptor);
                  if ((rc = ism_storePersist_getBuff(pStream, &pB, reqLen, msgType, &opcount, block)) != StoreRC_OK) return rc;
                  ismSTORE_putShort(pB.pPos, pOp->OperationType);
                  pLen = pB.pPos; pB.pPos += INT_SIZE; 
                  ismSTORE_putLong(pB.pPos, handle);
                  memcpy(pB.pPos, pOpDescriptor, sizeof(*pOpDescriptor) + pOpDescriptor->DataLength);

                  // Replace the temporary DataType with the exact DataType of the record
                  if (pOpDescriptor->DataType == ismSTORE_DATATYPE_NEWLY_HATCHED)
                  {
                     pTmp = pB.pPos + offsetof(ismStore_memDescriptor_t, DataType);
                     ismSTORE_putShort(pTmp, pOp->CreateRecord.DataType);
                  }
                  pB.pPos += sizeof(*pOpDescriptor) + pOpDescriptor->DataLength;
                  ismSTORE_putInt(pLen, pB.pPos - pLen - INT_SIZE);
                  opcount++;
                  TRACE(9,"persistHA: off %lu, opType %u, opLen %lu\n",(pLen-pB.pBuffer),pOp->OperationType,pB.pPos - pLen - INT_SIZE);
                  handle = pOpDescriptor->NextHandle;
               }

               break;

            case Operation_DeleteRecord:
               handle = pOp->DeleteRecord.Handle;
               genId = ismSTORE_EXTRACT_GENID(handle);
               while ((offset=ismSTORE_EXTRACT_OFFSET(handle)) > 0)
               {
                  if (genId == pTran->GenId || genId == ismSTORE_MGMT_GEN_ID)
                  {
                     if ((rc = ism_storePersist_getBuff(pStream, &pB, 64, msgType, &opcount, block)) != StoreRC_OK) return rc;
                     ismSTORE_putShort(pB.pPos, pOp->OperationType);
                     pLen = pB.pPos; pB.pPos += INT_SIZE; 
                     ismSTORE_putLong(pB.pPos, handle);
                     ismSTORE_putInt(pLen, pB.pPos - pLen - INT_SIZE);
                     opcount++;
                     genBase = ism_store_getBaseAddr(genId) ; 
                     pOpDescriptor = (ismStore_memDescriptor_t *)(genBase + offset);
                     if (ismSTORE_IS_SPLITITEM(pOpDescriptor->DataType))
                     {
                        pSplit = (ismStore_memSplitItem_t *)(pOpDescriptor + 1);
                        handle = pSplit->hLargeData;
                        genId = ismSTORE_EXTRACT_GENID(handle);
                     }
                     else
                     {
                        handle = pOpDescriptor->NextHandle;
                     }
                  }
                  else
                  {
                     break;
                  }
               }
               break;

            case Operation_UpdateRecord:
            case Operation_UpdateRecordAttr:
            case Operation_UpdateRecordState:

               if ((rc = ism_storePersist_getBuff(pStream, &pB, 64, msgType, &opcount, block)) != StoreRC_OK) return rc;
               handle = pOp->UpdateRecord.Handle;
               ismSTORE_putShort(pB.pPos, pOp->OperationType);
               pLen = pB.pPos; pB.pPos += INT_SIZE; 
               if (pOp->OperationType == Operation_UpdateRecord)
               {
                  ismSTORE_putLong(pB.pPos, handle);
                  ismSTORE_putLong(pB.pPos, pOp->UpdateRecord.Attribute);
                  ismSTORE_putLong(pB.pPos, pOp->UpdateRecord.State);
               }
               else
               {
                  ismSTORE_putLong(pB.pPos, handle);
                  if (pOp->OperationType == Operation_UpdateRecordAttr)
                  {
                     ismSTORE_putLong(pB.pPos, pOp->UpdateRecord.Attribute);
                  }
                  else
                  {
                     ismSTORE_putLong(pB.pPos, pOp->UpdateRecord.State);
                  }
               }
               ismSTORE_putInt(pLen, pB.pPos - pLen - INT_SIZE);
               opcount++;
               break;

            case Operation_UpdateRefState:

               if ((rc = ism_storePersist_getBuff(pStream, &pB, 64, msgType, &opcount, block)) != StoreRC_OK) return rc;
               handle = pOp->UpdateReference.Handle;
               genId = ismSTORE_EXTRACT_GENID(handle);
               offset = ismSTORE_EXTRACT_OFFSET(handle);
               chunkOff = ismSTORE_ROUND(offset, ismStore_memGlobal.MgmtSmallGranuleSizeBytes);
               pOpDescriptor = (ismStore_memDescriptor_t *)(tranBase + chunkOff);  // It is assumed here that both the transaction records and the refStates are in the Mgmt.
               pRefStateChunk = (ismStore_memRefStateChunk_t *)(pOpDescriptor + 1);

               ismSTORE_putShort(pB.pPos, pOp->OperationType);
               pLen = pB.pPos; pB.pPos += INT_SIZE; 
               ismSTORE_putLong(pB.pPos, handle);
               ismSTORE_putLong(pB.pPos, pRefStateChunk->OwnerHandle);
               ismSTORE_putLong(pB.pPos, pRefStateChunk->BaseOrderId);
               ismSTORE_putChar(pB.pPos, pOp->UpdateReference.State);
               ismSTORE_putInt(pLen, pB.pPos - pLen - INT_SIZE);
               opcount++;
               break;

            case Operation_CreateState:

               if ((rc = ism_storePersist_getBuff(pStream, &pB, 64, msgType, &opcount, block)) != StoreRC_OK) return rc;
               handle = pOp->CreateState.Handle;
               genId = ismSTORE_EXTRACT_GENID(handle);
               offset = ismSTORE_EXTRACT_OFFSET(handle);
               chunkOff = ismSTORE_ROUND(offset, ismStore_memGlobal.MgmtGranuleSizeBytes);
               pOpDescriptor = (ismStore_memDescriptor_t *)(tranBase + chunkOff);  // It is assumed here that both the transaction records and the states are in the Mgmt.
               pStateChunk = (ismStore_memStateChunk_t *)(pOpDescriptor + 1);
               pStateObject = (ismStore_memState_t *)(tranBase + offset);  // It is assumed here that both the transaction records and the states are in the Mgmt.
               ismSTORE_putShort(pB.pPos, pOp->OperationType);
               pLen = pB.pPos; pB.pPos += INT_SIZE; 
               ismSTORE_putLong(pB.pPos, handle);
               ismSTORE_putLong(pB.pPos, pStateChunk->OwnerHandle);
               ismSTORE_putInt(pB.pPos, pStateObject->Value);
               ismSTORE_putChar(pB.pPos, ismSTORE_STATEFLAG_VALID);
               ismSTORE_putInt(pLen, pB.pPos - pLen - INT_SIZE);
               opcount++;
               break;

            case Operation_DeleteState:

               if ((rc = ism_storePersist_getBuff(pStream, &pB, 64, msgType, &opcount, block)) != StoreRC_OK) return rc;
               handle = pOp->DeleteState.Handle;
               ismSTORE_putShort(pB.pPos, pOp->OperationType);
               pLen = pB.pPos; pB.pPos += INT_SIZE; 
               ismSTORE_putLong(pB.pPos, handle);
               ismSTORE_putInt(pLen, pB.pPos - pLen - INT_SIZE);
               opcount++;
               break;

            case Operation_UpdateActiveOid:

               if ((rc = ism_storePersist_getBuff(pStream, &pB, 64, msgType, &opcount, block)) != StoreRC_OK) return rc;
               ismSTORE_putShort(pB.pPos, pOp->OperationType);
               pLen = pB.pPos; pB.pPos += INT_SIZE; 
               ismSTORE_putLong(pB.pPos, pOp->UpdateActiveOid.OwnerHandle + sizeof(ismStore_memDescriptor_t) + offsetof(ismStore_memSplitItem_t, MinActiveOrderId));
               ismSTORE_putLong(pB.pPos, pOp->UpdateActiveOid.OrderId);
               ismSTORE_putInt(pLen, pB.pPos - pLen - INT_SIZE);
               opcount++;
               break;

            default:
               TRACE(1, "Failed to encode a store-transaction operation for persistence, because the operation type %d is not valid\n", pOp->OperationType);
               return StoreRC_SystemError;
         }
      }

      if (pDescriptor->NextHandle == ismSTORE_NULL_HANDLE)
      {
         break;
      }
      pDescriptor = (ismStore_memDescriptor_t *)(tranBase + ismSTORE_EXTRACT_OFFSET(pDescriptor->NextHandle));
   }

   // Send the last fragment
   // requiredLength = 0 means that this is the last fragment.
   // pBuffer = Null means that the store-transaction is empty
   if (!pB.pBuffer)
   {
      ismStore_persistInfo_t *pPersist = pStream->pPersist;
      ismSTORE_MUTEX_LOCK(&pStream->Mutex);
      if ( pPersist->curCB->pCallback )
      {
        if ( pPersist->NumCBs >= PERSIST_MAX_CBs )
        {
         #if USE_STATS
          pPersist->numFullCB++ ; 
         #endif
          if ( (rc = ism_store_persistWaitForSpace(pStream)) != ISMRC_OK )
          {
             ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);
             return rc ; 
          }
        }
        pPersist->CBs[pPersist->NumCBs++] = pPersist->curCB[0] ; 
        pPersist->State |= PERSIST_STATE_PENDING ;
        pInfo->go2Work = 1;
        pthread_cond_signal(pInfo->cond);
        if ( block )
          rc = ism_storePersist_completeST_(pStream);
      }
     #if USE_STATS
      else
        pPersist->numCommit++ ; 
     #endif
      pInfo->emptCBs++ ; 
      ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);
      TRACE(8, "The store-transaction is empty. No persist message was persisted, MsgSqn %lu\n",pPersist->MsgSqn);
      rc = StoreRC_HA_MsgUnsent;
   }
   else if ((rc = ism_storePersist_getBuff(pStream, &pB, 0, msgType, &opcount, block)) == StoreRC_OK)
   {
      TRACE(9, "A persist ST message (MsgType %u, MsgSqn %lu, FragsCount %u) has been persisted\n",
            msgType, pStream->pPersist->MsgSqn - 1, pStream->pPersist->FragSqn);
   }

   return rc;
}


int ism_storePersist_writeST(ismStore_memStream_t *pStream,int block)
{
  int rc;
//ismSTORE_MUTEX_LOCK(&pStream->Mutex);
  rc = ism_storePersist_writeST_(pStream,block);
//ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);
  return rc;
}

static int ism_storePersist_writeGenST_(ismStore_memStream_t *pStream,int block,ismStore_GenId_t genId,uint8_t genIndex,ismStore_memHAMsgType_t msgType)
{
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memGenHeader_t  *pGenHeader, *pGenHeaderPrev;
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_memGenIdChunk_t *pGenIdChunk;
   ismStore_Handle_t offset, handle;
   persistBuff_t pB={0};
   char *pLen;
   int rc;
   uint32_t opcount, dataLength;

   ism_storePersist_completeST(pStream);

   if ((rc = ism_storePersist_getBuff(pStream, &pB, 64, msgType, &opcount, block)) != StoreRC_OK) return rc;

   pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress;
   pGenHeader = (genId == ismSTORE_MGMT_GEN_ID ? (ismStore_memGenHeader_t *)pMgmtHeader : (ismStore_memGenHeader_t *)ismStore_memGlobal.InMemGens[genIndex].pBaseAddress);
   TRACE(5,"%s: genId=%hu, genIndex=%hu, msgType=%d, pMgmtHeader=%p, pGenHeader=%p\n",__FUNCTION__,genId, genIndex, msgType, pMgmtHeader, pGenHeader);

   // Generation information
   ismSTORE_putShort(pB.pPos, Operation_Null);
   pLen = pB.pPos; pB.pPos += INT_SIZE; 
   ismSTORE_putShort(pB.pPos, genId);
   ismSTORE_putChar(pB.pPos, genIndex);
   ismSTORE_putInt(pLen, pB.pPos - pLen - INT_SIZE);
   opcount++;

   // Generation header
   if (msgType == StoreHAMsg_CreateGen ||
       msgType == StoreHAMsg_WriteGen  ||
       msgType == StoreHAMsg_ActivateGen)
   {
      ismSTORE_putShort(pB.pPos, Operation_CreateRecord);
      pLen = pB.pPos; pB.pPos += INT_SIZE; 
      offset = pMgmtHeader->InMemGenOffset[genIndex] ; 
      ismSTORE_putLong(pB.pPos, offset);
      memcpy(pB.pPos, ismStore_memGlobal.InMemGens[genIndex].pBaseAddress, offsetof(ismStore_memGenHeader_t, Reserved));
      TRACE(9,"%s: off=%lu, base=%p, off=%lu\n",__FUNCTION__,offset,ismStore_memGlobal.InMemGens[genIndex].pBaseAddress,(ismStore_memGlobal.InMemGens[genIndex].pBaseAddress-ismStore_memGlobal.pStoreBaseAddress));
      ism_store_persistPrintGenHeader(ismStore_memGlobal.InMemGens[genIndex].pBaseAddress,__LINE__);
      pB.pPos += offsetof(ismStore_memGenHeader_t, Reserved);
      ismSTORE_putInt(pLen, pB.pPos - pLen - INT_SIZE);
      opcount++;              
   }

   // Generation state
   if (msgType == StoreHAMsg_WriteGen)
   {
      ismStore_GenId_t genIndexPrev = (genIndex + ismStore_memGlobal.InMemGensCount - 1) % ismStore_memGlobal.InMemGensCount;
      pGenHeaderPrev = (ismStore_memGenHeader_t *)ismStore_memGlobal.InMemGens[genIndexPrev].pBaseAddress;
      if (pGenHeaderPrev->State == ismSTORE_GEN_STATE_ACTIVE)
      {
         ismSTORE_putShort(pB.pPos, Operation_CreateRecord);
         pLen = pB.pPos; pB.pPos += INT_SIZE; 
         offset = pMgmtHeader->InMemGenOffset[genIndexPrev] ; 
         ismSTORE_putLong(pB.pPos, offset);
         memcpy(pB.pPos, ismStore_memGlobal.InMemGens[genIndexPrev].pBaseAddress, offsetof(ismStore_memGenHeader_t, Reserved));
         TRACE(9,"%s: off=%lu, base=%p, off=%lu\n",__FUNCTION__,offset,ismStore_memGlobal.InMemGens[genIndexPrev].pBaseAddress,(ismStore_memGlobal.InMemGens[genIndexPrev].pBaseAddress-ismStore_memGlobal.pStoreBaseAddress));
         ism_store_persistPrintGenHeader(ismStore_memGlobal.InMemGens[genIndexPrev].pBaseAddress,__LINE__);
         pB.pPos += offsetof(ismStore_memGenHeader_t, Reserved);
         ismSTORE_putInt(pLen, pB.pPos - pLen - INT_SIZE);
         opcount++;              
         pInfo->writeGenMsg |= 2 ; 
      }
   }

   // Management generation header
   if (msgType == StoreHAMsg_CreateGen ||
       msgType == StoreHAMsg_DeleteGen ||
       msgType == StoreHAMsg_AssignRsrvPool ||
       msgType == StoreHAMsg_WriteGen ||
       msgType == StoreHAMsg_ActivateGen)
   {
      ismSTORE_putShort(pB.pPos, Operation_CreateRecord);
      pLen = pB.pPos; pB.pPos += INT_SIZE; 
      ismSTORE_putLong(pB.pPos, 0);
      memcpy(pB.pPos, pMgmtHeader, offsetof(ismStore_memMgmtHeader_t, Reserved2));
      ism_store_persistPrintMgmtHeader(pB.pPos,__LINE__);
      pB.pPos += offsetof(ismStore_memMgmtHeader_t, Reserved2);
      ismSTORE_putInt(pLen, pB.pPos - pLen - INT_SIZE);
      opcount++;
   }

   // Generation Ids chunks
   if (msgType == StoreHAMsg_CreateGen ||
       msgType == StoreHAMsg_DeleteGen)
   {
      handle = pMgmtHeader->GenIdHandle;
      while (handle != ismSTORE_NULL_HANDLE)
      {
         pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(handle);
         pGenIdChunk = (ismStore_memGenIdChunk_t *)(pDescriptor + 1);
         dataLength = sizeof(ismStore_memDescriptor_t) + sizeof(ismStore_memGenIdChunk_t) - sizeof(ismStore_GenId_t) + pGenIdChunk->GenIdCount * sizeof(ismStore_GenId_t);
         if ((rc = ism_storePersist_getBuff(pStream, &pB, dataLength, msgType, &opcount, block)) != StoreRC_OK) return rc;
         ismSTORE_putShort(pB.pPos, Operation_CreateRecord);
         pLen = pB.pPos; pB.pPos += INT_SIZE; 
         ismSTORE_putLong(pB.pPos, ismSTORE_EXTRACT_OFFSET(handle));
         memcpy(pB.pPos, pDescriptor, dataLength);
         pB.pPos += dataLength;
         ismSTORE_putInt(pLen, pB.pPos - pLen - INT_SIZE);
         opcount++;
         handle = pDescriptor->NextHandle;
         ism_store_persistPrintGidChunk(pDescriptor,__LINE__);
      }
   }


   // Send the last fragment
   // requiredLength = 0 means that this is the last fragment.
   rc = ism_storePersist_getBuff(pStream, &pB, 0, msgType, &opcount, block);

   return rc;
}

static int ism_storePersist_writeActiveOid_(ismStore_memStream_t *pStream, ismStore_Handle_t ownerHandle, uint64_t minActiveOrderId)
{
   ismStore_memHAMsgType_t msgType = StoreHAMsg_UpdateActiveOid;
   persistBuff_t pB={0};
   char *pLen;
   int rc;
   uint32_t opcount;

   if ((rc = ism_storePersist_getBuff(pStream, &pB, 64, msgType, &opcount, 0)) != StoreRC_OK) return rc;

   ismSTORE_putShort(pB.pPos, Operation_UpdateActiveOid);
   pLen = pB.pPos; pB.pPos += INT_SIZE; 
   ismSTORE_putLong(pB.pPos, ownerHandle + sizeof(ismStore_memDescriptor_t) + offsetof(ismStore_memSplitItem_t, MinActiveOrderId));
   ismSTORE_putLong(pB.pPos, minActiveOrderId);
   ismSTORE_putInt(pLen, pB.pPos - pLen - INT_SIZE);
   opcount++;

   rc = ism_storePersist_getBuff(pStream, &pB, 0, msgType, &opcount, 0);

   return rc;
}

int ism_storePersist_writeActiveOid(ismStore_Handle_t ownerHandle, uint64_t minActiveOrderId)
{
   ismStore_memStream_t *pStream;
   int32_t rc ;

   if ( pInfo->init < PERSIST_STATE_STARTED )
   {
      TRACE(5,"%s was called before ism_storePersist_start(): owner=%p, mid=%lu\n",__FUNCTION__,(void *)ownerHandle, minActiveOrderId) ; 
      return StoreRC_OK;
   }

   pStream = ismStore_memGlobal.pStreams[ismStore_memGlobal.hInternalStream];
//ismSTORE_MUTEX_LOCK(&pStream->Mutex);
   rc = ism_storePersist_writeActiveOid_(pStream, ownerHandle, minActiveOrderId) ; 
//ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);

   return rc ; 
}

int ism_storePersist_writeGenST(int block,ismStore_GenId_t genId,uint8_t genIndex,ismStore_memHAMsgType_t msgType)
{
   ismStore_memStream_t *pStream;
   int32_t rc ;

   if ( pInfo->init < PERSIST_STATE_STARTED )
   {
      TRACE(5,"%s was called before ism_storePersist_start(): genId=%u, genIndex=%u, msgType=%u\n",__FUNCTION__,genId,genIndex,msgType);
      return StoreRC_OK;
   }

   pStream = ismStore_memGlobal.pStreams[ismStore_memGlobal.hInternalStream];
// ismSTORE_MUTEX_LOCK(&pStream->Mutex);
   rc = ism_storePersist_writeGenST_(pStream, block, genId, genIndex, msgType) ; 
// ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);

   return rc ; 
}

int ism_storePersist_onGenClosed(int genIndex)
{
  TRACE(5,"%s entry: genIndex=%u, curI=%u, curJ=%u, iState=%x, jState=%x\n",__FUNCTION__,genIndex,pInfo->curI,pInfo->curJ,pInfo->iState,pInfo->jState);
  pthread_mutex_lock(pInfo->lock) ; 
  if ( ++pInfo->genClosed > 1 )
  {
    TRACE(1,"Another genTranPhase is called for BEFORE being able to lock the store!!! , shuting down the server !!! \n");
    su_sleep(10,SU_MIL) ; 
    ism_common_shutdown(1);
  }
  pthread_cond_signal(pInfo->cond);
  pthread_mutex_unlock(pInfo->lock) ; 
  return StoreRC_OK ; 
}

int ism_storePersist_onGenWrite(int genIndex, int writeMsg)
{
  TRACE(5,"%s entry: genIndex=%u\n",__FUNCTION__,genIndex);
  if ( pInfo->init == PERSIST_STATE_STARTED )
  {
    pthread_mutex_lock(pInfo->lock) ; 
    if ( writeMsg ) pInfo->writeGenMsg |= 1 ; 
    pthread_cond_signal(pInfo->cond);
    pthread_mutex_unlock(pInfo->lock) ; 
  }
  else
  {
     TRACE(5,"%s was called before ism_storePersist_start(): genIndex=%u, writeMsg=%u\n",__FUNCTION__,genIndex,writeMsg);
  }
  return StoreRC_OK ; 
}

int ism_storePersist_onGenCreate(int genIndex)
{
  TRACE(5,"%s entry: genIndex=%u\n",__FUNCTION__,genIndex);
  if ( pInfo->init == PERSIST_STATE_STARTED )
  {
    pthread_mutex_lock(pInfo->lock) ; 
    pInfo->writeGenMsg |= 2 ; 
    pthread_cond_signal(pInfo->cond);
    pthread_mutex_unlock(pInfo->lock) ; 
  }
  else
  {
     TRACE(5,"%s was called before ism_storePersist_start(): genIndex=%u\n",__FUNCTION__,genIndex);
  }
  return StoreRC_OK ; 
}

static void ism_storePersist_writeCB(ismStore_GenId_t gid, int32_t rc, ismStore_DiskGenInfo_t *dgi, void *pCtx)
{
  int ns, i;
  ismStore_DiskTaskParams_t *dtp = (ismStore_DiskTaskParams_t *)pCtx ; 

  TRACE(5,"%s called with file= %s , rc= %d\n",__FUNCTION__,dtp->File,rc);
  if ( dtp->BufferParams->pBuffer )
  {
    // pBuffer comes from mmapped memory or will be realloced to memaligned memory
    // so must use a raw free as no eyecatcher will be included
    ism_common_free_raw(ism_memory_store_misc,dtp->BufferParams->pBuffer) ;
    dtp->BufferParams->pBuffer = NULL ; 
  }
  if ( rc != StoreRC_OK )
  {
    if ( rc == StoreRC_Disk_TaskCancelled || rc == StoreRC_Disk_TaskInterrupted )
    {
      TRACE(5,"CheckPoint task for cycleId %u has been cancelled or interrupted\n",pInfo->PState[0].cycleId);
    }
    else
    {
      TRACE(1,"CheckPoint task for cycleId %u has failed with error code %d\n",pInfo->PState[0].cycleId, rc);
    }
    ns = -1 ; 
  }
  else
    ns = 1 ; 

  i = ( dtp == pInfo->DiskTaskParams_CPG ) ; 
  pthread_mutex_lock(pInfo->lock) ; 
  pInfo->fileWriteState[i] = ns; 
  pthread_cond_signal(pInfo->cond);
  pthread_mutex_unlock(pInfo->lock) ; 
}

static void ism_storePersist_readCB(ismStore_GenId_t gid, int32_t rc, ismStore_DiskGenInfo_t *dgi, void *pCtx)
{
  int ns, i;
  ismStore_DiskTaskParams_t *dtp = (ismStore_DiskTaskParams_t *)pCtx ; 

  TRACE(5,"%s called with file= %s , rc= %d\n",__FUNCTION__,dtp->File,rc);
  if ( rc != StoreRC_OK )
  {
    if ( rc == StoreRC_Disk_TaskCancelled || rc == StoreRC_Disk_TaskInterrupted )
    {
      TRACE(1,"PersistRecovery read for file %s has been cancelled or interrupted\n",dtp->File);
    }
    else
    {
      TRACE(1,"PersistRecovery read for file %s has failed with error code %d\n",dtp->File, rc);
    }
    ns = -1 ; 
  }
  else
    ns = 1 ; 

  i = ( dtp == pInfo->DiskTaskParams_CPG ) ; 
  pthread_mutex_lock(pInfo->lock) ; 
  pInfo->fileReadState[i] = ns; 
  pthread_cond_signal(pInfo->cond);
  pthread_mutex_unlock(pInfo->lock) ; 
}

int ism_storePersist_prepareCP(int cI, int cJ)
{
  int rc ; 
  ismStore_memMgmtHeader_t *pMgmtHeader;
  ismStore_memGenHeader_t *pGenHeader;
  ismStore_DiskTaskParams_t *dtp ;
  ismStore_DiskBufferParams_t *bp ;

  dtp = pInfo->DiskTaskParams_CPM ; 
  memset(dtp, 0, sizeof(ismStore_DiskTaskParams_t)) ; 
  dtp->fCancelOnTerm = 0 ; 
  dtp->Priority      = 0 ; 
  dtp->GenId         = 0 ; 
  dtp->Callback      = ism_storePersist_writeCB ; 
  dtp->pContext      = dtp ; 
  dtp->Path          = pInfo->di->path ; 
  dtp->File          = pInfo->pFiles[cI][cJ].CPM_fn ; 
  bp = dtp->BufferParams ; 
  pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress;
  if ( cI != pMgmtHeader->ActiveGenIndex )
  {
    TRACE(1,"%s: @@@ cI (%u) != pMgmtHeader->ActiveGenIndex (%u) \n",__FUNCTION__,cI,pMgmtHeader->ActiveGenIndex);
  }
  if ( su_mutex_tryLock(&ismStore_memGlobal.StreamsMutex, 250) )
  {
    char desc[256];
    TRACE(3," Failed to lock StreamsMutex for 250 ms, the mutex seems to be locked by %s ; compacting the mgmt w/o locking...\n",
          ism_common_show_mutex_owner(&ismStore_memGlobal.StreamsMutex, desc,sizeof(desc)));
    rc = ism_storeDisk_compactGenerationData((void *)pMgmtHeader, bp) ;
  }
  else
  {
    rc = ism_storeDisk_compactGenerationData((void *)pMgmtHeader, bp) ; 
    pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
  } 
  TRACE(5,"%s: After ism_storeDisk_compactGenerationData: rc=%d, path=%s, file=%s, buff=%p, blen=%lu\n",__FUNCTION__,rc,
        dtp->Path, dtp->File, dtp->BufferParams->pBuffer, dtp->BufferParams->BufferLength);      
  if ( rc != StoreRC_OK ) return rc ; 

  dtp = pInfo->DiskTaskParams_CPG ; 
  memset(dtp, 0, sizeof(ismStore_DiskTaskParams_t)) ; 
  dtp->fCancelOnTerm = 0 ; 
  dtp->Priority      = 0 ; 
  dtp->GenId         = 0 ; 
  dtp->Callback      = ism_storePersist_writeCB ; 
  dtp->pContext      = dtp ; 
  dtp->Path          = pInfo->di->path ; 
  dtp->File          = pInfo->pFiles[cI][cJ].CPG_fn ; 
  bp = dtp->BufferParams ; 
  pGenHeader = (ismStore_memGenHeader_t *)ismStore_memGlobal.InMemGens[cI].pBaseAddress;
  rc = ism_storeDisk_compactGenerationData((void *)pGenHeader, bp) ;
    TRACE(1,"%s: ism_storeDisk_compactGenerationData: rc=%d, path=%s, file=%s, buff=%p, blen=%lu\n",__FUNCTION__,rc,
          dtp->Path, dtp->File, dtp->BufferParams->pBuffer, dtp->BufferParams->BufferLength);      

  return rc ; 
}

int ism_storePersist_writeCP(void)
{
  int rc ;
  ismStore_DiskTaskParams_t *dtp ;

  pInfo->fileWriteState[0] = 0 ; 
  dtp = pInfo->DiskTaskParams_CPM ; 
  rc = ism_storeDisk_writeFile(dtp) ; 
  if ( rc != StoreRC_OK )
  {
    TRACE(1,"%s: ism_storeDisk_writeFile failed, rc=%d, path=%s, file=%s, buff=%p, blen=%lu\n",__FUNCTION__,rc,
          dtp->Path, dtp->File, dtp->BufferParams->pBuffer, dtp->BufferParams->BufferLength);      
    return rc ; 
  }

  pInfo->fileWriteState[1] = 0 ; 
  dtp = pInfo->DiskTaskParams_CPG ; 
  rc = ism_storeDisk_writeFile(dtp) ; 
  if ( rc != StoreRC_OK )
  {
    TRACE(1,"%s: ism_storeDisk_writeFile failed, rc=%d, path=%s, file=%s, buff=%p, blen=%lu\n",__FUNCTION__,rc,
          dtp->Path, dtp->File, dtp->BufferParams->pBuffer, dtp->BufferParams->BufferLength);      
    return rc ; 
  }

  return rc ; 
}

int ism_storePersist_doRecovery(void)
{
  int rc, cI, cJ, iok, i, cid;
  size_t fsize, nIo=0 ; 
  uint64_t gs[2];
  persistFiles_t *pF ; 
  char *fn;
  void *genData ; 
  ismStore_DiskBufferParams_t *bp ; 
  ismStore_DiskTaskParams_t *dtp ; 
  ismStore_memGenHeader_t *pGenHeader ;
  persistReplay_t pr[1];

  cI = pInfo->PState[0].startGen ; 
  cJ = pInfo->PState[0].startFile[cI] ; 
  cid= pInfo->PState[0].cycleId ; 
  pF = &pInfo->pFiles[cI][cJ] ; 
  TRACE(5,"%s: cI=%u, cJ=%u, cid=%u\n",__FUNCTION__,cI,cJ,cid);

  ism_store_memGetGenSizes(gs, gs+1);

  rc = StoreRC_OK ; 
  do
  {
    iok = 0 ; 
    for ( i=0 ; i<2 ; i++ )
    {
      if ( i )
      {
        fn  = pF->CPG_fn ; 
        dtp = pInfo->DiskTaskParams_CPG ; 
      }
      else
      {
        fn  = pF->CPM_fn ; 
        dtp = pInfo->DiskTaskParams_CPM ; 
      }
      if ( (rc = ism_storeDisk_getFileSize(pInfo->di->path, fn, &fsize)) != StoreRC_OK )
      {
        TRACE(1,"Failed to get size of %s/%s for persistence, rc=%d.\n",pInfo->di->path,fn,rc);
        break ; 
      }
      memset(dtp, 0, sizeof(ismStore_DiskTaskParams_t)) ; 
      dtp->fCancelOnTerm = 0 ; 
      dtp->Priority      = 0 ; 
      dtp->GenId         = 0 ; 
      dtp->Callback      = ism_storePersist_readCB ; 
      dtp->pContext      = dtp ; 
      dtp->Path          = pInfo->di->path ; 
      dtp->File          = fn ; 
      bp = dtp->BufferParams ; 
      if (!(bp->pBuffer = ism_common_malloc_noeyecatcher(fsize)) )
      {
        TRACE(1,"Failed to allocate %lu bytes, rc=%d.\n",fsize,errno);
        rc = StoreRC_AllocateError ;
        break ; 
      }
      iok++ ; 
      bp->BufferLength = fsize ; 
      pInfo->fileReadState[i] = 0 ; 
      TRACE(5,"%s: calling ism_storeDisk_readFile for %s of size %lu\n",__FUNCTION__,fn,fsize);
      rc = ism_storeDisk_readFile(dtp) ; 
      if ( rc != StoreRC_OK )
      {
        TRACE(1,"ism_storeDisk_readFile failed for %s/%s, rc=%d.\n",pInfo->di->path,fn,rc);
        break ; 
      }
    }
    if ( i < 2 )
    {
      if ( iok > 0 )
        ism_common_free_raw(ism_memory_store_misc,pInfo->DiskTaskParams_CPM->BufferParams->pBuffer) ;
      if ( iok > 1 )
        ism_common_free_raw(ism_memory_store_misc,pInfo->DiskTaskParams_CPG->BufferParams->pBuffer) ;
      break ; 
    }
    iok = 0 ; 
    for ( i=0 ; i<2 ; i++ )
    {
      if ( i )
      {
        if ((rc = ism_store_memValidate()) != ISMRC_OK)  // after the mgmt has been loaded to mem
        {
          TRACE(1,"ism_store_memValidate() failed, rc=%d.\n",rc);
          break ; 
        }
        pGenHeader = (ismStore_memGenHeader_t *)ismStore_memGlobal.InMemGens[cI].pBaseAddress;
        dtp = pInfo->DiskTaskParams_CPG ; 
      }
      else
      {
        pGenHeader = (ismStore_memGenHeader_t *)ismStore_memGlobal.pStoreBaseAddress ;
        dtp = pInfo->DiskTaskParams_CPM ; 
      }
      pthread_mutex_lock(pInfo->lock) ; 
      while ( !pInfo->goDown && pInfo->fileReadState[i] == 0 )
        pthread_cond_wait(pInfo->cond, pInfo->lock);
      pthread_mutex_unlock(pInfo->lock) ; 
      bp = dtp->BufferParams ; 
      genData = bp->pBuffer ; 
      if ( pInfo->goDown || pInfo->fileReadState[i] != 1 )
      {
        if ( pInfo->fileReadState[i] == -1 )
          ism_common_free_memaligned(ism_memory_store_misc,genData) ;
        rc = StoreRC_SystemError ; 
        break ; 
      }
      bp->BufferLength = gs[i] ; 
      bp->pBuffer      = (void *)pGenHeader ; 
      TRACE(5,"%s: calling ism_storeDisk_expandGenerationData for file %s of size %lu\n",__FUNCTION__,dtp->File,bp->BufferLength);
      rc = ism_storeDisk_expandGenerationData(genData, bp) ; 
      ism_common_free_memaligned(ism_memory_store_misc,genData) ;
      if ( rc != StoreRC_OK)
      {
        TRACE(1,"ism_storeDisk_expandGenerationData() failed for %s, rc=%d.\n",dtp->File,rc);
        break ; 
      }
      if ( i )
      {
        ism_store_memInitGenStruct(cI);
        ism_store_persistPrintGenHeader(pGenHeader,__LINE__);
      }
      else
        ism_store_persistPrintMgmtHeader(pGenHeader,__LINE__);
    }
    if ( i < 2 )
      break ; 

    {
      ismStore_memMgmtHeader_t *pMgmtHeader;
      pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress ;
      if ( pMgmtHeader->NextAvailableGenId != ismSTORE_RSRV_GEN_ID && (ismStore_memGlobal.PersistRecoveryFlags&1) )
      {
        TRACE(1,"Creating the pMgmtHeader->NextAvailableGenId (%u) on genIndex %u\n",pMgmtHeader->NextAvailableGenId, 1-cI);
        ism_store_memCreateGeneration(1-cI,pMgmtHeader->NextAvailableGenId);
        ismStore_memGlobal.PersistCreatedGenId = pMgmtHeader->NextAvailableGenId;
      }
    }

    i = ismStore_memGlobal.StreamsSize ; 
    if (!(pInfo->pChs = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,33),i * sizeof(ismStore_memHAChannel_t *))) )
    {
      rc = StoreRC_AllocateError ;
      break ; 
    }
    pInfo->pChsSize = i ; 
    memset(pInfo->pChs, 0, (i * sizeof(ismStore_memHAChannel_t *))) ; 

    pr->cI = cI ; 
    pr->cJ = cJ ; 
    pr->cId= cid; 
    TRACE(5,"%s: calling ism_store_persistReplayFile for cI=%u, cJ=%u, cid=%u\n",__FUNCTION__,cI,cJ,cid);
    rc = ism_store_persistReplayFile(pr) ; 
    if ( rc != StoreRC_OK )
    {
      TRACE(1,"ism_store_persistReplayFile failed: cI=%u, cJ=%u, cId=%u, bId=%u, nIo=%lu\n",
                                               pr->cI,pr->cJ,pr->cId,pr->bId,pr->nIo) ; 
      break ; 
    }
    nIo += pr->nIo ;
    if ( pr->nIo )
    {
      INC_CID(cid) ;
      cJ ^= 1 ; 
      pr->cI = cI ; 
      pr->cJ = cJ ; 
      pr->cId= cid; 
      TRACE(5,"%s: calling ism_store_persistReplayFile for cI=%u, cJ=%u, cid=%u\n",__FUNCTION__,cI,cJ,cid);
      rc = ism_store_persistReplayFile(pr) ; 
      if ( rc != StoreRC_OK )
      {
        TRACE(1,"ism_store_persistReplayFile failed: cI=%u, cJ=%u, cId=%u, bId=%u, nIo=%lu\n",
                                                 pr->cI,pr->cJ,pr->cId,pr->bId,pr->nIo) ; 
        break ; 
      }
      nIo += pr->nIo ;
      if (!pr->nIo )
      {
        if (!(--cid) )
          cid-- ; 
      }
    }

    if ( pInfo->PState[0].genTr )
    do
    {
      ismStore_memMgmtHeader_t *pMgmtHeader;
      pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;
      pGenHeader = (ismStore_memGenHeader_t *)ismStore_memGlobal.InMemGens[1-cI].pBaseAddress;
      if (pMgmtHeader->ActiveGenId != pGenHeader->GenId ||
          pMgmtHeader->ActiveGenIndex != 1-cI           ||
          pGenHeader->State != ismSTORE_GEN_STATE_ACTIVE )
      {
        TRACE(1,"%s: Sanity check failed: genId: %u<>%u ; index: %u<>%u, state: %u<>%u\n",__FUNCTION__,
          pMgmtHeader->ActiveGenId,pGenHeader->GenId,pMgmtHeader->ActiveGenIndex,cI,pGenHeader->State,ismSTORE_GEN_STATE_ACTIVE);
        rc = StoreRC_SystemError ; 
        break ; 
      }

      INC_CID(cid) ;
      cI ^= 1 ; 
      cJ = pInfo->PState[0].startFile[cI] ; 
      pr->cI = cI ; 
      pr->cJ = cJ ; 
      pr->cId= cid; 
      rc = ism_store_persistReplayFile(pr) ; 
      if ( rc != StoreRC_OK )
      {
        TRACE(1,"ism_store_persistReplayFile failed: cI=%u, cJ=%u, cId=%u, bId=%u, nIo=%lu\n",
                                                 pr->cI,pr->cJ,pr->cId,pr->bId,pr->nIo) ; 
        break ; 
      }
      nIo += pr->nIo ;
      if ( pr->nIo )
      {
        INC_CID(cid) ;
        cJ ^= 1 ; 
        pr->cI = cI ; 
        pr->cJ = cJ ; 
        pr->cId= cid; 
        rc = ism_store_persistReplayFile(pr) ; 
        if ( rc != StoreRC_OK )
        {
          TRACE(1,"ism_store_persistReplayFile failed: cI=%u, cJ=%u, cId=%u, bId=%u, nIo=%lu\n",
                                                   pr->cI,pr->cJ,pr->cId,pr->bId,pr->nIo) ; 
          break ; 
        }
        nIo += pr->nIo ;
        if (!pr->nIo )
        {
          if (!(--cid) )
            cid-- ; 
        }
      }
    } while(0);
    ism_common_free(ism_memory_store_misc,pInfo->pChs) ; 
    pInfo->pChs = NULL ; 
  } while(0) ; 
  pInfo->recoveryDone = 1 ; 

  if ( rc == StoreRC_OK && !nIo )
    pInfo->needCP = 0 ; 

  pInfo->curI = cI ; 
  pInfo->curJ = cJ ; 

  return rc ; 
}

int ism_store_persistIO(int fd, char *buff, size_t batch, int ioIn)
{
  ssize_t bytes ; 
  size_t count ; 
  int job_errno, job_line ; 

  count = 0 ; 
  bytes = 0 ; 
  while ( bytes < batch && !pInfo->goDown )
  {
    bytes = ioIn ? read(fd, buff, batch) : write(fd, buff, batch) ;
    if ( bytes == batch )
    {
      if ( !ioIn && fdatasync(fd) < 0 )
      {
        job_errno = errno ; 
        job_line = __LINE__ ; 
        TRACE(1,"fdatasync failed at %d, errno=%d(%s)\n",__LINE__,errno,strerror(errno));
        su_sleep(1,SU_MIL);
      }
      return bytes ; 
    }
    else
    {
      int ok=0 ; 
      do
      {
        if ( ++count > 5000 )
        {
          job_errno = EIO ; 
          job_line = __LINE__ ; 
          break ; 
        }
        if ( bytes > 0 )
        {
          if ( ioIn )
          {
            if ( read(fd, buff, 1) == 0 )
            {
              job_errno = EIO ; 
              job_line = __LINE__ ; 
              break ; 
            }
          }
          else 
          {
            if ( write(fd, buff, 1) < 0 )
            {
              job_errno = errno ; 
              job_line = __LINE__ ; 
              break ; 
            }
          }
          bytes++ ; 
          if ( lseek(fd, -bytes, SEEK_CUR) == (off_t)(-1) )
          {
            job_errno = errno ; 
            job_line = __LINE__ ; 
            break ; 
          }
          bytes = 0 ; 
        }
        else
        if ( bytes < 0 )
        {
          if ( errno == EINTR )
            bytes = 0 ; 
          else
          {
            job_errno = errno ; 
            job_line = __LINE__ ; 
            break ; 
          }
        }
        else
        if ( ioIn )
          return 0 ; 
        if ( fdatasync(fd) < 0 )
          {
            job_errno = errno ; 
            job_line = __LINE__ ; 
            break ; 
          }
        ok++ ; 
      } while(0) ; 
      if ( !ok )
      {
        TRACE(1,"%s failed: op=%s, rc=%d (%s), pos=%d, batch=%lu\n",__FUNCTION__,(ioIn?"read":"write"),job_errno,strerror(job_errno),job_line,batch);
        return -1 ; 
      }
      su_sleep(4,SU_MIL);
    }
  }
  return 0 ; 
}

int ism_storePersist_checkStopToken(void)
{
  ismStore_memMgmtHeader_t *pMgmtHeader;

  pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress;
  return memcmp(pMgmtHeader->PersistToken,pInfo->PState[0].stopToken, sizeof(pInfo->PState[0].stopToken)) ; 
}

int ism_storePersist_calcStopToken(void *buff, size_t buffLen, uint8_t *token, int tokenLen)
{
  uint64_t ts = ism_common_currentTimeNanos() ^ pthread_self() ; 
  
  memcpy(token,&ts,tokenLen) ; 
  return StoreRC_OK;
}

int ism_storePersist_getAsyncCBStats(uint32_t *pTotalReadyCBs, uint32_t *pTotalWaitingCBs,
                                     uint32_t *pNumThreads,
                                     ismStore_AsyncThreadCBStats_t *pCBThreadStats)
{
    int rc = StoreRC_OK;

    if ( pInfo->goDown ||pInfo->init < PERSIST_STATE_STARTED )
    {
        rc = ISMRC_StoreNotAvailable;
    }

    if ( rc == StoreRC_OK)
    {
        if (pTotalReadyCBs != NULL)
        {
            *pTotalReadyCBs = __sync_add_and_fetch(&pInfo->numRCBs,0);
        }
        if (pTotalWaitingCBs != NULL)
        {
            pthread_mutex_lock(pInfo->lock);
            *pTotalWaitingCBs = pInfo->numWCBs;
            pthread_mutex_unlock(pInfo->lock);
        }

        //Do we have enough space to write out the per-Thread stats?
        if (*pNumThreads < pInfo->numTHrx)
        {
            //Tell the caller how many threads they need to provide for
            *pNumThreads = pInfo->numTHrx;
            rc = ISMRC_ArgNotValid;
        }
        else
        {
           //Let the caller know how many threads we have provided stats for
           *pNumThreads = pInfo->numTHrx;
        }
    }

    //Do we need to lock pInfo before looking at the different async callback structures? I don't think so b
    if (rc == StoreRC_OK)
    {
        int i;

        for ( i=0 ; i<pInfo->numTHrx ; i++ )
        {
            rcbQueue_t *pRcbQ = &(pInfo->rcbQ[i]);

            pthread_mutex_lock(pRcbQ->lock);
            *pCBThreadStats = pRcbQ->lastPeriodStats;

            //Number Ready/Processing/Waiting Callbacks return "live" not recorded during stats period
            pCBThreadStats->numProcessingCallbacks = pRcbQ->numPCBs;
            pCBThreadStats->numReadyCallbacks      = pRcbQ->numRCBs;
            pCBThreadStats->numWaitingCallbacks    = pRcbQ->numWCBs;

            pthread_mutex_unlock(pRcbQ->lock);

            pCBThreadStats++;
        }
    }

    return rc;
}

#ifdef __cplusplus
}
#endif
