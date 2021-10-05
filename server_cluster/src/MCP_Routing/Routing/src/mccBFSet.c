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
#include "mccBFSet.h"

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

struct mcc_bfs_BFSet_t
{
  mcc_hash_getAllValues_t    getHashValues ;
  size_t                     dataLen;
  uint8_t                   *data ; 
  mcc_bfs_BFLookupHandle_t  *user ; 
  uint32_t                  *lens ; 
  union
  {
   uint8_t                  *mask0_1 ; 
   uint16_t                 *mask0_2 ; 
   uint32_t                 *mask0_4 ; 
   uint64_t                 *mask0_8 ; 
  };
  mcc_bfs_BFSetParameters_t  params[1] ; 
  uint16_t                   numBits ; 
  uint16_t                   numBytes; 
  int                        numPos  ; 
  uint16_t                   mode ; 
  uint16_t                   nextI;    // MUST be tw LAST fields
  BFSet_rwlock_t             lock[1];
};

static uint8_t   mask0[8] = {0xfe,0xfd,0xfb,0xf7,0xef,0xdf,0xbf,0x7f} ; 
static uint8_t   mask1[8] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80} ; 

/*************************************************************************/

XAPI int mcc_bfs_createBFSet(mcc_bfs_BFSetHandle_t *phBFSetHandle, mcc_bfs_BFSetParameters_t *pBFSetParams)
{
  size_t size , *pc, cn[2] ;  
  int nb, nB, i,m, np;
  mcc_bfs_BFSetHandle_t pbf ; 
  void *ptr ; 

  if ( !phBFSetHandle || pBFSetParams->numBFs < 1 || pBFSetParams->maxBFLen < 1 )
    return ISMRC_Error ; 

  nb = (pBFSetParams->numBFs+7)&(~7) ; 
  nB = nb>>3 ; 
  for ( i=0,m=1 ; m<nB ; i++ , m <<= 1 )  ; // empty body ; 
  nb = m<<3 ; 
  nB = m ; 
  np = pBFSetParams->maxBFLen << 3 ; 
  size = sizeof(struct mcc_bfs_BFSet_t) ; 
  if (!(pbf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_cluster_misc,27),size)) )
    return ISMRC_AllocateError ; 
  memset(pbf,0,size) ; 
  if ( BFSet_rwlock_init(pbf->lock,NULL) )
  {
    ism_common_free(ism_memory_cluster_misc,pbf) ;
    return ISMRC_Error ; 
  }
  pc = pBFSetParams->dbg_cnt ; 
  cn[1] = __sync_add_and_fetch(&pc[1],size);
  m = 1 << (2 * (i<3?i:3) + 3) ;
  size = (nB * np) + (nb * sizeof(mcc_bfs_BFLookupHandle_t)) + (nb * sizeof(uint32_t)) + m ; 
  if ( posix_memalign(&ptr, getpagesize(), size) )
  {
    ism_common_free(ism_memory_cluster_misc,pbf) ; 
    return ISMRC_AllocateError ; 
  }
  cn[0] = __sync_add_and_fetch(&pc[0],1);
  cn[1] = __sync_add_and_fetch(&pc[1],size);
  memset(ptr,0,size) ; 
  memcpy(pbf->params, pBFSetParams, sizeof(mcc_bfs_BFSetParameters_t)) ; 
  pbf->numBits  = nb ; 
  pbf->numBytes = nB ; 
  pbf->numPos   = np ; 
  pbf->mode     = i<4 ? i : 4 ; 
  pbf->data     = (uint8_t  *)ptr ;
  pbf->dataLen  = size ;  
  pbf->user     = (mcc_bfs_BFLookupHandle_t  *)((uintptr_t)pbf->data + (nB * np)) ; 
  pbf->lens     = (uint32_t *)((uintptr_t)pbf->user + (nb * sizeof(mcc_bfs_BFLookupHandle_t))) ;
  switch (pbf->mode)
  {
   case 0 :
   {
     uint8_t b=1 ; 
     pbf->mask0_1 = (uint8_t *)((uintptr_t)pbf->lens + (nb * sizeof(uint32_t))) ; 
     for ( m=0 ; m<8 ; m++,b*=2 )
       pbf->mask0_1[m] = ~b ; 
     break ; 
   }
   case 1 :
   {
     uint16_t b=1 ; 
     pbf->mask0_2 = (uint16_t *)((uintptr_t)pbf->lens + (nb * sizeof(uint32_t))) ; 
     for ( m=0 ; m<16 ; m++,b*=2 )
       pbf->mask0_2[m] = ~b ; 
     break ; 
   }
   case 2 :
   {
     uint32_t b=1 ; 
     pbf->mask0_4 = (uint32_t *)((uintptr_t)pbf->lens + (nb * sizeof(uint32_t))) ; 
     for ( m=0 ; m<32 ; m++,b*=2 )
       pbf->mask0_4[m] = ~b ; 
     break ; 
   }
   default:
   {
     uint64_t b=1 ; 
     pbf->mask0_8 = (uint64_t *)((uintptr_t)pbf->lens + (nb * sizeof(uint32_t))) ; 
     for ( m=0 ; m<64 ; m++,b*=2 )
       pbf->mask0_8[m] = ~b ; 
     break ; 
   }
  }
#if 0
  ISM_HASH_TYPE_NONE               = 0,
  ISM_HASH_TYPE_CITY64_LC          = 1, /* City 64 bit, with linear combinations   */
  ISM_HASH_TYPE_CITY64_CH         	= 2, /* City 64 bit, seed chaining              */
  ISM_HASH_TYPE_MURMUR_x64_128_LC 	= 3, /* Murmur 64 bit, with linear combinations */
  ISM_HASH_TYPE_MURMUR_x64_128_CH 	= 4  /* Murmur 64 bit, seed chaining            */

  void mcc_hash_getAllValues_city64_LC(const char *pKey, size_t keyLen, int numValues, uint32_t maxValue, uint32_t *pResults);
  void mcc_hash_getAllValues_city64_LC_raw(const char *pKey, size_t keyLen, int numValues, uint32_t maxValue, uint32_t *pResults);
  void mcc_hash_getAllValues_city64_simple(const char *pKey, size_t keyLen, int numValues, uint32_t maxValue, uint32_t *pResults);

  void mcc_hash_getAllValues_murmur3_x86_32(const char *pKey, size_t keyLen, int numValues, uint32_t maxValue, uint32_t *pResults);
  void mcc_hash_getAllValues_murmur3_x64_128(const char *pKey, size_t keyLen, int numValues, uint32_t maxValue, uint32_t *pResults);
  void mcc_hash_getAllValues_murmur3_x64_128_raw(const char *pKey, size_t keyLen, int numValues, uint32_t maxValue, uint32_t *pResults);
  void mcc_hash_getAllValues_murmur3_x64_128_LC(const char *pKey, size_t keyLen, int numValues, uint32_t maxValue, uint32_t *pResults);

  void mcc_hash_getSingleValue_city64_simple(const char *pKey, size_t keyLen, uint32_t maxValue, char *pInput, uint32_t *pResult);
  void mcc_hash_getSingleValue_murmur3_x86_32(const char *pKey, size_t keyLen, uint32_t maxValue, char *pInput, uint32_t *pResult);
#endif

  switch (pBFSetParams->hashType)
  {
    case ISM_HASH_TYPE_CITY64_LC :
      pbf->getHashValues = mcc_hash_getAllValues_city64_LC;
      break;
    case ISM_HASH_TYPE_CITY64_CH :
      pbf->getHashValues = mcc_hash_getAllValues_city64_simple;
      break;
    case ISM_HASH_TYPE_MURMUR_x64_128_LC :
      pbf->getHashValues = mcc_hash_getAllValues_murmur3_x64_128_LC;
      break;
    case ISM_HASH_TYPE_MURMUR_x64_128_CH :
      pbf->getHashValues = mcc_hash_getAllValues_murmur3_x64_128;
      break;
    default :
      mcc_bfs_deleteBFSet(pbf) ;
      return ISMRC_Error ; 
  }
  TRACE(5,"%s: Memory_Allocation_Monitoring: instanceId=%u, nAdd=%lu, tAdd=%lu\n",__func__,pbf->params->id,cn[0],cn[1]);  
  *phBFSetHandle = pbf ; 
  return ISMRC_OK ; 
}

/*************************************************************************/

XAPI int mcc_bfs_deleteBFSet(mcc_bfs_BFSetHandle_t hBFSetHandle)
{
  size_t size, *pc, cn[2] ; 
  mcc_bfs_BFSetHandle_t pbf=hBFSetHandle ; 
  if (!pbf )
    return ISMRC_Error ; 
  BFSet_rwlock_destroy(pbf->lock) ;
  size = pbf->dataLen;
  pc = pbf->params->dbg_cnt ; 
  cn[1] = __sync_add_and_fetch(&pc[3],size);
  ism_common_free_raw(ism_memory_cluster_misc,pbf->data) ;
  size = sizeof(struct mcc_bfs_BFSet_t);
  cn[0] = __sync_add_and_fetch(&pc[2],1);
  cn[1] = __sync_add_and_fetch(&pc[3],size);
  TRACE(5,"%s: Memory_Allocation_Monitoring: instanceId=%u, nSub=%lu, tSub=%lu\n",__func__,pbf->params->id,cn[0],cn[1]); 
  ism_common_free(ism_memory_cluster_misc,pbf) ;     
  return ISMRC_OK ; 
}

/*************************************************************************/

static int mcc_bfs_resizeBFSet(mcc_bfs_BFSetHandle_t hBFSetHandle,mcc_bfs_BFSetParameters_t *pBFSetParams)
{
  int rc=ISMRC_OK , i ; 
  mcc_bfs_BFSetHandle_t pbf=hBFSetHandle, nbf ; 
  uint8_t *ptr, *ntr, *etr ; 
  size_t len ; 

  if (!pbf )
    return ISMRC_Error ; 

//BFSet_rwlock_wrlock(pbf->lock) ; 
  do
  {
    if ( pBFSetParams->numBFs < pbf->params->numBFs || pBFSetParams->maxBFLen < pbf->params->maxBFLen )
    {
      rc = ISMRC_ArgNotValid ; 
      break ; 
    }
    if ( pBFSetParams->numBFs ==pbf->params->numBFs && pBFSetParams->maxBFLen ==pbf->params->maxBFLen )
    {
      rc = ISMRC_OK ; 
      break ; 
    }
    if ( (rc = mcc_bfs_createBFSet(&nbf, pBFSetParams)) != ISMRC_OK )
      break ; 
    ptr = pbf->data ; 
    ntr = nbf->data ; 
    etr = ptr + (pbf->numBytes * pbf->numPos) ; 
    for ( i=0 ; i<nbf->numPos ; i++ )
    {
      memcpy(ntr, ptr, pbf->numBytes) ; 
      ptr += pbf->numBytes ; 
      ntr += nbf->numBytes ; 
      if ( ptr >= etr )
        ptr = pbf->data ;
    }
    memcpy(nbf->user, pbf->user, pbf->numBits*sizeof(mcc_bfs_BFLookupHandle_t)) ; 
    memcpy(nbf->lens, pbf->lens, pbf->numBits*sizeof(uint32_t)) ; 
    ptr = pbf->data ; 
    len = pbf->dataLen ; 
    memcpy(pbf, nbf, offsetof(struct mcc_bfs_BFSet_t,nextI)) ; 
    nbf->data = ptr ; 
    nbf->dataLen = len ; 
    mcc_bfs_deleteBFSet(nbf) ; 
  } while(0) ; 
//BFSet_rwlock_unlock(pbf->lock) ;
  return rc ; 
}

/*************************************************************************/

static void mcc_bfs_setBF(mcc_bfs_BFSetHandle_t pbf, int BFIndex, const char *pBFBytes, int BFLen)
{
  int i,j,k,l,m ; 
  uint8_t b, *ptr ; 

  i = BFIndex ; 
  j = i&7 ; 
  i >>= 3 ; 
  for ( m=0 ; m<pbf->numPos ; )
  {
    for ( k=0 ; k<BFLen ; k++ )
    {
      for ( l=0 ; l<8 ; l++ )
      {
        b = pBFBytes[k] & mask1[l] ; 
        b >>= l ; 
        b <<= j ; 
        ptr = pbf->data + m * pbf->numBytes + i ; 
        ptr[0] &= mask0[j] ; 
        ptr[0] |= b ; 
        m++ ; 
      }
    }
  }
}     

/*************************************************************************/

XAPI int mcc_bfs_addBF(mcc_bfs_BFSetHandle_t hBFSetHandle, int BFIndex, const char *pBFBytes, int BFLen, mcc_bfs_BFLookupHandle_t hLookupHandle)
{
  int i , rc=ISMRC_OK ; 
  mcc_bfs_BFSetHandle_t pbf=hBFSetHandle ; 

  if ( !pbf )
    return ISMRC_ArgNotValid ; 

  BFSet_rwlock_wrlock(pbf->lock) ;
  do
  {
    if ( BFLen > pbf->params->maxBFLen || BFIndex >= pbf->numBits )
    {
      mcc_bfs_BFSetParameters_t params[1] ; 
      memcpy(params, pbf->params, sizeof(params)) ; 
      if ( params->maxBFLen < BFLen )
           params->maxBFLen = BFLen ;
      if ( params->numBFs   < BFIndex + 1 )
           params->numBFs   = BFIndex + 1 ;
      if ( (rc = mcc_bfs_resizeBFSet(pbf, params)) != ISMRC_OK )
        break ; 
    }
    i = BFIndex ; 
    if ( i >= pbf->nextI )
      pbf->nextI = i+1 ; 
    pbf->user[i] = hLookupHandle ; 
    pbf->lens[i] = BFLen << 3 ; 
    mcc_bfs_setBF(pbf, i, pBFBytes, BFLen) ; 
  } while(0) ; 
  BFSet_rwlock_unlock(pbf->lock) ;
  return rc ; 
}     

/*************************************************************************/

XAPI int mcc_bfs_replaceBF(mcc_bfs_BFSetHandle_t hBFSetHandle, int BFIndex, const char *pBFBytes, int BFLen)
{
  int i , rc=ISMRC_OK ; 
  mcc_bfs_BFSetHandle_t pbf=hBFSetHandle ; 
  if ( !pbf )
    return ISMRC_ArgNotValid ; 
  
  BFSet_rwlock_wrlock(pbf->lock) ;
  do
  {
    if ( BFLen > pbf->params->maxBFLen || BFIndex >= pbf->numBits )
    {
      mcc_bfs_BFSetParameters_t params[1] ; 
      memcpy(params, pbf->params, sizeof(params)) ; 
      if ( params->maxBFLen < BFLen )
           params->maxBFLen = BFLen ;
      if ( params->numBFs   < BFIndex + 1 )
           params->numBFs   = BFIndex + 1 ;
      if ( (rc = mcc_bfs_resizeBFSet(pbf, params)) != ISMRC_OK )
        break ; 
    }
    i = BFIndex ; 
    if ( i >= pbf->nextI )
      pbf->nextI = i+1 ; 
    mcc_bfs_setBF(pbf, i, pBFBytes, BFLen) ; 
    pbf->lens[i] = BFLen << 3 ; 
  } while(0) ; 
  BFSet_rwlock_unlock(pbf->lock) ;
  return rc ; 
}

/*************************************************************************/

XAPI int mcc_bfs_updateBF(mcc_bfs_BFSetHandle_t hBFSetHandle, int BFIndex, const int *pUpdates, int updatesLen)
{
  int i,j,k,l,m,n ; 
  uint8_t *ptr ; 
  mcc_bfs_BFSetHandle_t pbf=hBFSetHandle ; 

  if ( !pbf )
    return ISMRC_ArgNotValid ; 

  BFSet_rwlock_wrlock(pbf->lock) ;
  if ( BFIndex >= pbf->numBits )
  {
    BFSet_rwlock_unlock(pbf->lock) ;
    return ISMRC_ArgNotValid ; 
  }
  i = BFIndex ; 
  n = pbf->lens[i] ; 
  j = i&7 ; 
  i >>= 3 ; 
  for ( l=0 ; l<pbf->numPos ; l += n )
  {
    for ( k=0 ; k<updatesLen ; k++ )
    {
      if ( pUpdates[k] > 0 )
      {
        m = pUpdates[k] - 1 ; 
        ptr = pbf->data + (l+m) * pbf->numBytes + i ; 
        ptr[0] |= mask1[j] ; 
      }
      else
      {
        m = -pUpdates[k] - 1 ; 
        ptr = pbf->data + (l+m) * pbf->numBytes + i ; 
        ptr[0] &= mask0[j] ; 
      }
    }
  }
  BFSet_rwlock_unlock(pbf->lock) ;
  return ISMRC_OK ; 
}

/*************************************************************************/

XAPI int mcc_bfs_deleteBF(mcc_bfs_BFSetHandle_t hBFSetHandle, int BFIndex)
{
  int i,j,k ; 
  uint8_t *ptr ; 
  mcc_bfs_BFSetHandle_t pbf=hBFSetHandle ; 

  if ( !pbf )
    return ISMRC_ArgNotValid ; 

  BFSet_rwlock_wrlock(pbf->lock) ;
  if ( BFIndex >= pbf->nextI )
  {
    BFSet_rwlock_unlock(pbf->lock) ;
    return ISMRC_ArgNotValid ; 
  }
  i = BFIndex ; 
  pbf->user[i] = NULL ; 
  pbf->lens[i] = 0 ; 
  j = i&7 ;
  i >>= 3 ; 
  for ( k=0 ; k<pbf->numPos ; k++ )
  {
    ptr = pbf->data + k * pbf->numBytes + i ; 
    ptr[0] &= mask0[j] ; 
  }
  BFSet_rwlock_unlock(pbf->lock) ;
  return ISMRC_OK ; 
}

/*************************************************************************/

XAPI int mcc_bfs_lookup(mcc_bfs_BFSetHandle_t hBFSetHandle, char *pTopic, size_t topicLen, uint8_t *skip,
                        mcc_bfs_BFLookupHandle_t *phResults, int resultsLen, int *pNumResults)
{
  int i,n=0, positionsLen, rc=ISMRC_OK ; 
  mcc_bfs_BFSetHandle_t pbf=hBFSetHandle ; 
  static void *mode[] = {&&case0, &&case1, &&case2, &&case3, &&case4};
  uint32_t pPositions[MCC_MAX_HASH_VALUES];
  

  BFSet_rwlock_rdlock(pbf->lock) ;

//  return ISMRC_ClusterArrayTooSmall ;
  positionsLen = pbf->params->numHashValues;
  pbf->getHashValues(pTopic, topicLen, positionsLen, pbf->numPos, pPositions);

  goto *mode[pbf->mode] ; 
  do
  {
    case0:
    {
      uint8_t *ptr, b ; 
      ptr = (uint8_t *)skip ; 
      b = 0xff & (~ptr[0]) ; 
      for ( i=0 ; i<positionsLen && b ; i++ )
      {
        ptr = pbf->data + pPositions[i] * pbf->numBytes ; 
        b &= ptr[0] ; 
      }
      for ( i=0 ; i<pbf->nextI && b ; i++ )
      {
        if ( b&1 )
        {
          if ( n >= resultsLen )
          {
            rc = ISMRC_ClusterArrayTooSmall ;
            break ; 
          }
          phResults[n++] = pbf->user[i] ; 
        }
        b >>= 1 ; 
      }
      break ; 
    }
    case1:
    {
      uint16_t *ptr, b ; 
      ptr = (uint16_t *)skip ; 
      b = 0xffff & (~ptr[0]) ; 
      for ( i=0 ; i<positionsLen && b ; i++ )
      {
        ptr = (uint16_t *)(pbf->data + pPositions[i] * pbf->numBytes) ; 
        b &= ptr[0] ; 
      }
      for ( i=0 ; i<pbf->nextI && b ; i++ )
      {
        if ( b&1 )
        {
          if ( n >= resultsLen )
          {
            rc = ISMRC_ClusterArrayTooSmall ;
            break ; 
          }
          phResults[n++] = pbf->user[i] ; 
        }
        b >>= 1 ; 
      }
      break ; 
    }
    case2:
    {
      uint32_t *ptr, b ; 
      ptr = (uint32_t *)skip ; 
      b = 0xffffffff & (~ptr[0]) ; 
      for ( i=0 ; i<positionsLen && b ; i++ )
      {
        ptr = (uint32_t *)(pbf->data + pPositions[i] * pbf->numBytes) ; 
        b &= ptr[0] ; 
      }
      for ( i=0 ; i<pbf->nextI && b ; i++ )
      {
        if ( b&1 )
        {
          if ( n >= resultsLen )
          {
            rc = ISMRC_ClusterArrayTooSmall ;
            break ; 
          }
          phResults[n++] = pbf->user[i] ; 
        }
        b >>= 1 ; 
      }
      break ; 
    }
    case3:
    {
      uint64_t *ptr, b ; 
      ptr = (uint64_t *)skip ; 
      b = 0xffffffffffffffffUL & (~ptr[0]) ; 
      for ( i=0 ; i<positionsLen && b ; i++ )
      {
        ptr = (uint64_t *)(pbf->data + pPositions[i] * pbf->numBytes) ; 
        b &= ptr[0] ; 
      }
      for ( i=0 ; i<pbf->nextI && b ; i++ )
      {
        if ( b&1 )
        {
          if ( n >= resultsLen )
          {
            rc = ISMRC_ClusterArrayTooSmall ;
            break ; 
          }
          phResults[n++] = pbf->user[i] ; 
        }
        b >>= 1 ; 
      }
      break ; 
    }
    case4:
    {
      uint j,k,l,m ; 
      uint64_t *ptr, b ; 
      mcc_bfs_BFLookupHandle_t  *user=pbf->user ; 
      k = pbf->numBytes >> 3 ; 
      l = pbf->nextI ; 
      for ( j=0 ; j<k && l && rc==ISMRC_OK ; j++,user+=64 )
      {
        ptr = (uint64_t *)skip ; 
        b = 0xffffffffffffffffUL & (~ptr[j]) ; 
        for ( i=0 ; i<positionsLen && b ; i++ )
        {
          ptr = (uint64_t *)(pbf->data + pPositions[i] * pbf->numBytes) ; 
          b &= ptr[j] ; 
        }
        m = l>64 ? 64 : l ; 
        l -= m ; 
        for ( i=0 ; i<m && b ; i++ )
        {
          if ( b&1 )
          {
            if ( n >= resultsLen )
            {
              rc = ISMRC_ClusterArrayTooSmall ;
              break ; 
            }
            phResults[n++] = pbf->user[i] ; 
          }
          b >>= 1 ; 
        }
      }
      break ; 
    }
  } while(0);
  *pNumResults = n ; 
  BFSet_rwlock_unlock(pbf->lock) ;
  return rc ; 
}
