/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <errno.h>
#include <time.h>

#ifndef XAPI
#define XAPI
#endif

#include <ismrc.h>
#include "mccWildcardBFSet.h"
#include "hashFunction.h"

#ifdef __cplusplus
extern "C" {
#endif

#define USE_LOCK 0

#if USE_LOCK
 #define BFSet_rwlock_t           pthread_rwlock_t
 #define BFSet_rwlock_init(x,y)   pthread_rwlock_init(x,y)
 #define BFSet_rwlock_destroy(x)  pthread_rwlock_destroy(x)
 #define BFSet_rwlock_wrlock(x)   pthread_rwlock_wrlock(x)
 #define BFSet_rwlock_rdlock(x)   pthread_rwlock_wrlock(x)
 #define BFSet_rwlock_unlock(x)   pthread_rwlock_wrlock(x)
#else
 #define BFSet_rwlock_t           int
 #define BFSet_rwlock_init(x,y)   0
 #define BFSet_rwlock_destroy(x)
 #define BFSet_rwlock_wrlock(x)
 #define BFSet_rwlock_rdlock(x)
 #define BFSet_rwlock_unlock(x)
#endif

#if 0

typedef struct mcc_wcbfs_WCBFSet_t  * mcc_wcbfs_WCBFSetHandle_t;
typedef struct mcc_BFLookup_t       * mcc_wcbfs_BFLookupHandle_t;

#endif

typedef struct
{
  uint64_t           patternId;      /* Pattern identifier                     */
  uint16_t           numPluses;      /* Number of pluses in pPlusLevels        */
  uint16_t          *pPlusLevels;    /* Array holding the plus levels          */
  uint16_t           hashLevel;      /* Level of hash (0 if none)              */
  uint16_t           patternLen;     /* Number of level in the pattern         */
} mcc_lus_Pattern_i;


typedef struct patLL_
{
  struct patLL_     *next ; 
  mcc_lus_Pattern_i  pat[1] ;
} patLL ; 

typedef struct
{
  patLL                      *f1stPat ; 
  char                       *BFBytes ; 
  size_t                      BFLen ; 
  size_t                      numPos ; 
  mcc_wcbfs_BFLookupHandle_t  user ; 
  mcc_hash_getAllValues_t     getHashValues ;
  int                         numHashValues;
  int                         state ;
} mcc_wcbf_t ; 

struct mcc_wcbfs_WCBFSet_t
{
  mcc_wcbf_t                    *wcbf ; 
  int                            nextI ; 
  int                            maxBFs;
  BFSet_rwlock_t                 lock[1];
};

static uint8_t   mask0[8] = {0xfe,0xfd,0xfb,0xf7,0xef,0xdf,0xbf,0x7f} ; 
static uint8_t   mask1[8] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80} ; 
static int mcc_wcbfs_addPattern_(mcc_wcbfs_WCBFSetHandle_t hWCBFSetHandle, int BFIndex, mcc_lus_Pattern_t *pPattern);

/*************************************************************************/

XAPI int mcc_wcbfs_createWCBFSet(mcc_wcbfs_WCBFSetHandle_t *phWCBFSetHandle)
{
  mcc_wcbfs_WCBFSetHandle_t pbf ; 
  int maxBFs = 64 ; 
  size_t size ; 

  if ( !phWCBFSetHandle )
    return ISMRC_Error ; 
  
  size = sizeof(struct mcc_wcbfs_WCBFSet_t) ;
  if (!(pbf = (mcc_wcbfs_WCBFSetHandle_t)ism_common_malloc(ISM_MEM_PROBE(ism_memory_cluster_misc,1),size)) )
    return ISMRC_AllocateError ; 
  memset(pbf,0,size) ; 
  size = maxBFs * sizeof(mcc_wcbf_t) ; 
  if (!(pbf->wcbf = (mcc_wcbf_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_cluster_misc,2),size)) )
  {
    ism_common_free(ism_memory_cluster_misc,pbf) ;
    return ISMRC_AllocateError ; 
  }
  memset(pbf->wcbf,0,size) ; 
  if ( BFSet_rwlock_init(pbf->lock,NULL) )
  {
    ism_common_free(ism_memory_cluster_misc,pbf->wcbf) ;
    ism_common_free(ism_memory_cluster_misc,pbf) ;
    return ISMRC_Error ; 
  }
  pbf->maxBFs = maxBFs ; 
  *phWCBFSetHandle = pbf ; 
  return ISMRC_OK ; 
}

/*************************************************************************/

XAPI int mcc_wcbfs_deleteWCBFSet(mcc_wcbfs_WCBFSetHandle_t hWCBFSetHandle)
{
  mcc_wcbfs_WCBFSetHandle_t pbf = hWCBFSetHandle ; 
  mcc_wcbf_t *wcbf ; 
  int i;

  if ( !hWCBFSetHandle )
    return ISMRC_Error ; 

  wcbf = pbf->wcbf ; 
  for ( i=0 ; i<pbf->nextI ; i++,wcbf++ )
  {
    if ( wcbf->state )
    {
      while ( wcbf->f1stPat )
      {
        patLL *p = wcbf->f1stPat ; 
        wcbf->f1stPat = p->next ; 
        ism_common_free(ism_memory_cluster_misc,p) ; 
      }
      if ( wcbf->state&1 )
        ism_common_free(ism_memory_cluster_misc,wcbf->BFBytes) ;
    }
  }
  ism_common_free(ism_memory_cluster_misc,pbf->wcbf) ; 
  BFSet_rwlock_destroy(pbf->lock) ; 
  ism_common_free(ism_memory_cluster_misc,pbf) ; 
  return ISMRC_OK ; 
}

/*************************************************************************/

XAPI int mcc_wcbfs_addBF(mcc_wcbfs_WCBFSetHandle_t hWCBFSetHandle, int BFIndex, mcc_hash_t *hashParams,
                       const char *pBFBytes, size_t BFLen, mcc_wcbfs_BFLookupHandle_t hLookupHandle)
{
  mcc_wcbfs_WCBFSetHandle_t pbf = hWCBFSetHandle ; 
  mcc_wcbf_t *wcbf ; 
  int i;
  int rc=ISMRC_OK ; 

  if ( !(hWCBFSetHandle) )
    return ISMRC_Error ; 

  BFSet_rwlock_wrlock(pbf->lock) ; 
  do
  {
    i = BFIndex ; 
    if ( i >= pbf->nextI )
    {
      if ( pbf->nextI >= pbf->maxBFs )
      {
        size_t size = pbf->maxBFs * 2 * sizeof(mcc_wcbf_t) ;
        if (!(wcbf = (mcc_wcbf_t *)ism_common_realloc(ISM_MEM_PROBE(ism_memory_cluster_misc,10),pbf->wcbf,size)) )
        {
          rc = ISMRC_AllocateError ; 
          break ; 
        }
        memset(wcbf+pbf->maxBFs,0,size/2) ; 
        pbf->wcbf = wcbf ; 
        pbf->maxBFs *= 2 ; 
      }
      pbf->nextI = i+1 ; 
    }
    wcbf = pbf->wcbf + i ; 
    if ( wcbf->state&1 )
    {
      if ( wcbf->user != hLookupHandle )
      {
        rc = ISMRC_Error ; 
        break ; 
      }
      if ( wcbf->BFLen != BFLen )
      {
        void *ptr = ism_common_realloc(ISM_MEM_PROBE(ism_memory_cluster_misc,11),wcbf->BFBytes, BFLen) ; 
        if ( !ptr )
        {
          rc = ISMRC_AllocateError ; 
          break ; 
        }
        wcbf->BFBytes = ptr ; 
      }
    }
    else
    {
      if (!wcbf->state )
        memset(wcbf,0,sizeof(mcc_wcbf_t)) ;
      if (!(wcbf->BFBytes = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_cluster_misc,12),BFLen)) )
      {
        rc = ISMRC_AllocateError ; 
        break ; 
      }
      wcbf->user = hLookupHandle ; 
      wcbf->state |= 1 ;
    }
    memcpy(wcbf->BFBytes,pBFBytes,BFLen) ;
    wcbf->BFLen = BFLen ;
    wcbf->numPos= BFLen<<3 ;
    wcbf->numHashValues = hashParams->numHashValues;
    switch (hashParams->hashType)
    {
      case ISM_HASH_TYPE_CITY64_LC :
        wcbf->getHashValues = mcc_hash_getAllValues_city64_LC;
        break;
      case ISM_HASH_TYPE_CITY64_CH :
        wcbf->getHashValues = mcc_hash_getAllValues_city64_simple;
        break;
      case ISM_HASH_TYPE_MURMUR_x64_128_LC :
        wcbf->getHashValues = mcc_hash_getAllValues_city64_simple;
        break;
      case ISM_HASH_TYPE_MURMUR_x64_128_CH :
        wcbf->getHashValues = mcc_hash_getAllValues_murmur3_x64_128;
        break;
      default :
        return ISMRC_Error ; 
    }
  } while(0);
  BFSet_rwlock_unlock(pbf->lock) ;
  return rc ; 
}  

/*************************************************************************/

XAPI int mcc_wcbfs_updateBF(mcc_wcbfs_WCBFSetHandle_t hWCBFSetHandle, int BFIndex,
                          const int *pUpdates, int updatesLen)
{
  int i = BFIndex ; 
  int j,k,l ; 
  mcc_wcbfs_WCBFSetHandle_t pbf = hWCBFSetHandle ; 
  mcc_wcbf_t *wcbf ; 
  int rc=ISMRC_OK ; 

  if ( !hWCBFSetHandle )
    return ISMRC_Error ; 

  BFSet_rwlock_wrlock(pbf->lock) ; 
  do
  {
    wcbf = pbf->wcbf + i ; 
    if (!(BFIndex < pbf->nextI && wcbf->state) )
    {
      rc = ISMRC_Error ; 
      break ; 
    }
    for ( j=0 ; j<updatesLen ; j++ )
    {
      if ( pUpdates[j] > 0 )
      {
        k = pUpdates[j] - 1 ; 
        l = k&7 ; 
        k >>= 3 ; 
        if ( k < wcbf->BFLen )
          wcbf->BFBytes[k] |= mask1[l] ;
        else
        	break;
      }
      else
      {
        k = -pUpdates[j] - 1 ; 
        l = k&7 ; 
        k >>= 3 ; 
        if ( k < wcbf->BFLen )
          wcbf->BFBytes[k] &= mask0[l] ;
        else
         	break;
      }
    }
    if ( j<updatesLen )
    {
      rc = ISMRC_Error ;
    }
  } while(0);
  BFSet_rwlock_unlock(pbf->lock) ;
  return rc ; 
}

/*************************************************************************/

XAPI int mcc_wcbfs_deleteBF(mcc_wcbfs_WCBFSetHandle_t hWCBFSetHandle, int BFIndex)
{
  int i = BFIndex ; 
  mcc_wcbfs_WCBFSetHandle_t pbf = hWCBFSetHandle ; 
  mcc_wcbf_t *wcbf ; 
  int rc=ISMRC_OK ; 

  if ( !hWCBFSetHandle )
    return ISMRC_Error ; 

  BFSet_rwlock_wrlock(pbf->lock) ; 
  do
  {
    wcbf = pbf->wcbf + i ; 
    if (!(BFIndex < pbf->nextI && wcbf->state) )
    {
      rc = ISMRC_Error ; 
      break ; 
    }
    while ( wcbf->f1stPat )
    {
      patLL *p = wcbf->f1stPat ; 
      wcbf->f1stPat = p->next ; 
      ism_common_free(ism_memory_cluster_misc,p) ; 
    }
    ism_common_free(ism_memory_cluster_misc,wcbf->BFBytes) ; 
    memset(wcbf,0,sizeof(mcc_wcbf_t)) ; 
  } while(0);
  BFSet_rwlock_unlock(pbf->lock) ;
  return rc ; 
}

/*************************************************************************/

static int mcc_wcbfs_addPattern_(mcc_wcbfs_WCBFSetHandle_t hWCBFSetHandle, int BFIndex, mcc_lus_Pattern_t *pPattern)
{
  int i ;
  mcc_wcbfs_WCBFSetHandle_t pbf = hWCBFSetHandle ; 
  mcc_wcbf_t *wcbf ; 
  int rc=ISMRC_OK ; 

  if ( !hWCBFSetHandle )
    return ISMRC_Error ; 

  do
  {
    patLL *pll, *qll ; 
    size_t size ;
    i = BFIndex ;
    if ( i >= pbf->nextI )
    {
      if ( pbf->nextI >= pbf->maxBFs )
      {
        size = pbf->maxBFs * 2 * sizeof(mcc_wcbf_t) ;
        if (!(wcbf = (mcc_wcbf_t *)ism_common_realloc(ISM_MEM_PROBE(ism_memory_cluster_misc,15),pbf->wcbf,size)) )
        {
          rc = ISMRC_AllocateError ;
          break ;
        }
        memset(wcbf+pbf->maxBFs,0,size/2) ;
        pbf->wcbf = wcbf ;
        pbf->maxBFs *= 2 ;
      }
      pbf->nextI = i+1 ;
    }
    wcbf = pbf->wcbf + i ;

    if (!wcbf->state )
      memset(wcbf,0,sizeof(mcc_wcbf_t)) ;
    wcbf->state |= 2 ;
    for ( pll=wcbf->f1stPat, qll=NULL ; pll ; qll=pll, pll=pll->next )
    {
      if ( pll->pat->patternId == pPattern->patternId )
      {
        if ( pPattern->numPluses > pll->pat->numPluses )
        {
          void *ptr ; 
          size = sizeof(patLL) + pPattern->numPluses * sizeof(uint16_t) ;
          if (!(ptr = ism_common_realloc(ISM_MEM_PROBE(ism_memory_cluster_misc,16),pll,size)) )
          {
            rc = ISMRC_AllocateError ; 
            break ; 
          }
          pll = (patLL *)ptr ; 
          if ( qll )
            qll->next = pll ; 
          else
            wcbf->f1stPat = pll ;
        }
        memcpy(pll->pat, pPattern, sizeof(mcc_lus_Pattern_t)) ; 
        pll->pat->pPlusLevels = (uint16_t *)((uintptr_t)pll + sizeof(patLL)) ; 
        memcpy(pll->pat->pPlusLevels, pPattern->pPlusLevels, pPattern->numPluses * sizeof(uint16_t)) ; 
        break ; 
      }
    }
    if (!pll )
    {
      size = sizeof(patLL) + pPattern->numPluses * sizeof(uint16_t) ;
      if (!(pll = (patLL *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_cluster_misc,17),size)) )
      {
        rc = ISMRC_AllocateError ; 
        break ; 
      }
      memcpy(pll->pat, pPattern, sizeof(mcc_lus_Pattern_t)) ; 
      pll->pat->pPlusLevels = (uint16_t *)((uintptr_t)pll + sizeof(patLL)) ; 
      memcpy(pll->pat->pPlusLevels, pPattern->pPlusLevels, pPattern->numPluses * sizeof(uint16_t)) ; 
      pll->next = wcbf->f1stPat ; 
      wcbf->f1stPat = pll ; 
    }
  } while(0);
  return rc ;
}

XAPI int mcc_wcbfs_addPattern(mcc_wcbfs_WCBFSetHandle_t hWCBFSetHandle, int BFIndex,
                              mcc_lus_Pattern_t *pPattern)
{
  int rc;
  BFSet_rwlock_wrlock(pbf->lock) ;
  rc = mcc_wcbfs_addPattern_(hWCBFSetHandle, BFIndex, pPattern);
  BFSet_rwlock_unlock(pbf->lock) ;
  return rc ; 
}


/*************************************************************************/

XAPI int mcc_wcbfs_deletePattern(mcc_wcbfs_WCBFSetHandle_t hWCBFSetHandle, int BFIndex,
                                 uint64_t patternId)
{
  int i = BFIndex ; 
  mcc_wcbfs_WCBFSetHandle_t pbf = hWCBFSetHandle ; 
  mcc_wcbf_t *wcbf ; 
  int rc=ISMRC_OK ; 

  if ( !hWCBFSetHandle )
    return ISMRC_Error ; 

  BFSet_rwlock_wrlock(pbf->lock) ; 
  do
  {
    patLL *pll, *qll ; 
    wcbf = pbf->wcbf + i ; 
    if (!(BFIndex < pbf->nextI && (wcbf->state&2)) )
    {
      rc = ISMRC_Error ; 
      break ; 
    }
    for ( pll=wcbf->f1stPat, qll=NULL ; pll ; qll=pll, pll=pll->next )
    {
      if ( pll->pat->patternId == patternId )
      {
        if ( qll )
          qll->next = pll->next ; 
        else
          wcbf->f1stPat = pll->next ;
        ism_common_free(ism_memory_cluster_misc,pll) ; 
        break ; 
      }
    }
    if (!pll )
    {
      rc = ISMRC_Error ; 
      break ; 
    }
  } while(0);
  BFSet_rwlock_unlock(pbf->lock) ;
  return rc ; 
}

/*************************************************************************/

XAPI int mcc_wcbfs_lookup(mcc_wcbfs_WCBFSetHandle_t hWCBFSetHandle, char *pTopic, int topicLen, uint8_t *skip, 
                        mcc_wcbfs_BFLookupHandle_t *phResults, int resultsLen, int *pNumResults)
{
  int i,n ; 
  mcc_wcbfs_WCBFSetHandle_t pbf = hWCBFSetHandle ; 
  mcc_wcbf_t *wcbf ; 
  int rc=ISMRC_OK ; 

  if ( !hWCBFSetHandle )
    return ISMRC_Error ; 

  BFSet_rwlock_rdlock(pbf->lock) ; 
  do
  {
    int j1,k1;
    wcbf = pbf->wcbf ; 
    n = 0 ; 
    for ( i=0 ; i<pbf->nextI && rc==ISMRC_OK ; i++,wcbf++ )
    {
      j1 = i >> 3 ; 
      k1 = i &  7 ; 
      if ( (wcbf->state&3)==3 && !(skip[j1]&mask1[k1]) )
      {
        patLL *pll ; 
        int nl=0 , nh=wcbf->numHashValues ;
        for ( pll=wcbf->f1stPat ; pll && rc==ISMRC_OK ; pll=pll->next )
        {
          char t[topicLen], *p, *q, *u2 ; 
          uint32_t hashes[nh];
          int j,k,l,m ; 
          if ( nl > pll->pat->hashLevel  &&  pll->pat->hashLevel ) continue ; 
          if ( nl > pll->pat->patternLen && !pll->pat->hashLevel ) continue ; 
          j = k = 0 ; 
          l = 1 ; 
          m = 0 ; 
          p = pTopic ; 
          u2= p + topicLen ; 
          q = t ; 
          while ( p < u2 )
          {
            if ( j < pll->pat->numPluses && l == pll->pat->pPlusLevels[j] )
            {
              j++ ; 
              *q++ = '+' ; 
              do
              {
                if ( *p++ == '/' )
                {
                  l++ ; 
                  *q++ = '/' ; 
                  break ; 
                }
              } while ( p < u2 )  ; 
            }
            else
            if ( l == pll->pat->hashLevel )
            {
              *q++ = '#' ; 
              break ; 
            }
            else
            if ( l > pll->pat->patternLen )
            {
              m++ ; 
              break ; 
            }
            else
            {
              do
              {
                if ( (*q++ = *p++) == '/' )
                {
                  l++ ; 
                  break ; 
                }
              } while ( p < u2 )  ; 
            }
          }
          if ( nl < l )
               nl = l ;
          if ( m || j < pll->pat->numPluses || l < pll->pat->hashLevel )
            continue ; 
          l = q - t ; 
          wcbf->getHashValues(t,l, nh, wcbf->numPos, hashes);
          for ( m=0 ; m<nh ; m++ )
          {
            j = hashes[m] & 7 ; 
            k = hashes[m] >> 3 ; 
            if ( !(wcbf->BFBytes[k] & mask1[j]) )
              break ; 
          }
          if ( m>=nh )
          {
            if ( n >= resultsLen )
            {
              rc = ISMRC_ClusterArrayTooSmall ; 
              break ; 
            }
            phResults[n++] = wcbf->user ; 
            break ; 
          }
        }
      }
    }
  } while(0);
  BFSet_rwlock_unlock(pbf->lock) ;
  *pNumResults = n ; 
  return rc ; 
}
