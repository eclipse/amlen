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
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/vfs.h>
#include <stdbool.h>
#include "storeDiskUtils.h"

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER ;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER ;
double t0;

#include "stubFuncs.c"

double sysTime(void)
{
  struct timeval tv[1];
  gettimeofday(tv,NULL);
  return (double)tv->tv_sec + (double)tv->tv_usec/1e6 - t0 ; 
}

int gen_stat[2] ; 

typedef struct
{
  int act_type ; 
  int gen_id ; 
} ActInfo ; 

void mycb(ismStore_GenId_t GenId, int32_t retcode, ismStore_DiskGenInfo_t *pDiskGenInfo,void *pContext)
{
  ActInfo ai[1] ; 

  if ( !pContext )
  {
    printf("_dbg_%d: pContext is NULL!\n",__LINE__);
    return;
  }
  memcpy(ai,pContext, sizeof(ActInfo)) ;
  free(pContext) ; 
  printf("_dbg_%d: mycb: GenId= %hu , act_type= %d , gen_id= %d , rc= %d\n",__LINE__,GenId, ai->act_type, ai->gen_id, retcode);
  if ( ai->act_type < 3 && GenId != ai->gen_id )
  {
    printf("_dbg_%d: GenId != ai->gen_id : %hu %d\n",__LINE__,GenId, ai->gen_id);
    return;
  }

  pthread_mutex_lock(&lock) ; 
  if ( ai->act_type < 3 ) gen_stat[(GenId&1)] |= ai->act_type ; 
  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&lock) ; 
}


int main(int argc, char *argv[])
{
  int i , rc ; 
  uint64_t *buffs[2], off, val, gen_size, tot_size ; 
  ismStore_GenId_t GenId ; 
  ismStore_DiskParameters_t dp[1] ; 
  ismStore_DiskBufferParams_t bp[1] ;
  ismStore_DiskTaskParams_t dtp[1] ; 
  ActInfo *ai ; 

  gen_size = 0 ; 
  if ( argc > 1 ) gen_size = atoi(argv[1]);
  if ( gen_size <= 0 ) gen_size = 1 ; 
  gen_size <<= 30 ; 

  tot_size = 0 ; 
  if ( argc > 2 ) tot_size = atoi(argv[2]);
  if ( tot_size <= 0 ) tot_size = 16 ; 
  tot_size <<= 30 ; 

  i = getpagesize() ; 
  if ( (rc = posix_memalign((void **)(buffs+0), i, gen_size)) ||
       (rc = posix_memalign((void **)(buffs+1), i, gen_size)) )
  {
    printf("_dbg_%d: posix_memalign failed rc= %d\n",__LINE__,rc);
    exit(-1);
  }

  dp->TransferBlockSize = 1 << 16 ;
  if ( argc > 3 )
    strcpy(dp->RootPath, argv[3]) ; 
  else
    strcpy(dp->RootPath, "/sdc") ; 
  dp->ClearStoredFiles = 1 ; 

  rc = ism_storeDisk_init(dp);
  if ( rc != StoreRC_OK )
  {
    printf("_dbg_%d: ism_storeDisk_init failed rc= %d\n",__LINE__,rc);
    exit(-1);
  }

  memset(bp, 0, sizeof(bp));
  t0 = sysTime();
  gen_stat[0] = 1 ; 
  gen_stat[1] = 1 ; 
  for ( i=0, GenId=0, val=0 ; val < tot_size ; val += sizeof(uint64_t) )
  {
    if ( val / gen_size > GenId )
    {
      bp->pBuffer = (char *)buffs[i] ; 
      bp->BufferLength = gen_size ; 
      ai = malloc(sizeof(ActInfo)) ; 
      ai->act_type = 1 ; 
      ai->gen_id   = GenId ; 

      pthread_mutex_lock(&lock) ; 
      gen_stat[i] &= (~1) ; 
      pthread_mutex_unlock(&lock) ; 
      memset(dtp,0,sizeof(dtp));
      memcpy(dtp->BufferParams, bp, sizeof(bp));
      dtp->Priority = 0 ; 
      dtp->GenId = GenId; 
      dtp->Callback = mycb;
      dtp->pContext = ai;
      printf("_dbg_%d: calling ism_storeDisk_writeGeneration for GenId %hu ; %f\n",__LINE__,GenId,sysTime());
      rc = ism_storeDisk_writeGeneration(dtp) ; 
      if ( rc )
      {
        printf("_dbg_%d: ism_storeDisk_writeGeneration failed rc= %d errno=%d\n",__LINE__,rc,errno);
        exit(-1);
      }
      GenId++ ; 
      i = (GenId&1) ; 
      pthread_mutex_lock(&lock) ; 
      while (!(gen_stat[i]&1) )
      {
        printf("_dbg_%d: start waiting for GenId %u to be written to disk ; %f\n",__LINE__,GenId-2,sysTime());
        pthread_cond_wait(&cond,&lock) ; 
      }
      pthread_mutex_unlock(&lock) ; 
      if ( GenId > 1 ) printf("_dbg_%d: finish waiting for GenId %u to be written to disk ; %f\n",__LINE__,GenId-2,sysTime());
    }
    off = (val % gen_size)/sizeof(uint64_t) ; 
    buffs[i][off] = val ; 
  }
  gen_stat[0] = 2 ; 
  gen_stat[1] = 2 ; 
  while ( val > 0 )
  {
    val -= sizeof(uint64_t) ; 
    if ( val / gen_size < GenId )
    {
      if ( GenId > 1 )
      {
        bp->pBuffer = (char *)buffs[i] ; 
        bp->BufferLength = gen_size ; 
        ai = malloc(sizeof(ActInfo)) ; 
        ai->act_type = 2 ; 
        ai->gen_id   = GenId-2 ; 
  
        pthread_mutex_lock(&lock) ; 
        gen_stat[i] &= (~2) ; 
        pthread_mutex_unlock(&lock) ; 
        memset(dtp,0,sizeof(dtp));
        memcpy(dtp->BufferParams, bp, sizeof(bp));
        dtp->Priority = 0 ; 
        dtp->GenId = GenId-2; 
        dtp->Callback = mycb;
        dtp->pContext = ai;
        printf("_dbg_%d: calling ism_storeDisk_readGeneration for GenId %u ; %f\n",__LINE__,GenId-2,sysTime());
        rc = ism_storeDisk_readGeneration(dtp) ; 
        if ( rc )
        {
          printf("_dbg_%d: ism_storeDisk_readGeneration failed rc= %d errno=%d\n",__LINE__,rc,errno);
          exit(-1);
        }
      }
      GenId-- ; 
      i = (GenId&1) ; 
      pthread_mutex_lock(&lock) ; 
      while (!(gen_stat[i]&2) )
      {
        printf("_dbg_%d: start waiting for GenId %hu to be read from disk ; %f\n",__LINE__,GenId,sysTime());
        pthread_cond_wait(&cond,&lock) ; 
      }
      pthread_mutex_unlock(&lock) ; 
      printf("_dbg_%d: finish waiting for GenId %hu to be read from disk ; %f\n",__LINE__,GenId,sysTime());
    }
    off = (val % gen_size)/sizeof(uint64_t) ; 
    if ( buffs[i][off] != val ) 
    {
      printf("_dbg_%d: wrong value! %d %hu %lu %lu %lu \n",__LINE__,i,GenId,off,val,buffs[i][off]);
      exit(-1);
    }
  }
  for ( i=(tot_size/gen_size), GenId=0 ; GenId <= i ; GenId++ )
  {
    ai = malloc(sizeof(ActInfo)) ; 
    ai->act_type = 2 ; 
    ai->gen_id   = GenId ; 

    pthread_mutex_lock(&lock) ; 
    gen_stat[(GenId&1)] &= (~2) ; 
    pthread_mutex_unlock(&lock) ; 
    memset(dtp,0,sizeof(dtp));
    dtp->Priority = 0 ; 
    dtp->GenId = GenId; 
    dtp->Callback = mycb;
    dtp->pContext = ai;
    printf("_dbg_%d: calling ism_storeDisk_deleteGeneration for GenId %hu ; %f\n",__LINE__,GenId,sysTime());
    rc = ism_storeDisk_deleteGeneration(dtp) ; 
    if ( rc )
    {
      printf("_dbg_%d: ism_storeDisk_deleteGeneration failed rc= %d errno=%d\n",__LINE__,rc,errno);
      exit(-1);
    }
    pthread_mutex_lock(&lock) ; 
    while (!(gen_stat[(GenId&1)]&2) )
    {
      printf("_dbg_%d: start waiting for GenId %hu to be deleted from disk ; %f\n",__LINE__,GenId,sysTime());
      pthread_cond_wait(&cond,&lock) ; 
    }
    pthread_mutex_unlock(&lock) ; 
    printf("_dbg_%d: finish waiting for GenId %hu to be deleted from disk ; %f\n",__LINE__,GenId,sysTime());
  }
  ai = malloc(sizeof(ActInfo)) ; 
  ai->act_type = 3 ; 
  memset(dtp,0,sizeof(dtp));
  dtp->Priority = 0 ; 
  dtp->Callback = mycb;
  dtp->pContext = ai;
  rc = ism_storeDisk_term(dtp) ; 
  if ( rc != StoreRC_OK )
  {
    printf("_dbg_%d: ism_storeDisk_term failed rc= %d\n",__LINE__,rc);
    exit(-1);
  }
  pthread_cond_wait(&cond,&lock) ; 
  return 0;
}
