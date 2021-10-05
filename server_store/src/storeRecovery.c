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
/* Module Name: storeRecovery.c                                      */
/*                                                                   */
/* Description: Recovery module for Store Component of ISM           */
/*                                                                   */
/*********************************************************************/
#define TRACE_COMP Store
#include "storeInternal.h"
#include "storeMemory.h"
#include "storeDiskUtils.h"
#include "storeRecovery.h"
#include "storeUtils.h"

#define USE_REC_TIME 0
#define USE_NEXT_OWNER 1


#if USE_NEXT_OWNER
typedef struct
{
  uint64_t *Owners ; 
  uint64_t OwnersSize ; 
  uint64_t OwnersInd ;
} ismStore_ownerByInd_t ; 
#endif

typedef struct ismStore_memGenInfo_t
{
  struct ismStore_memGenInfo_t    *next ; 
  double                           useTime ; 
  uint64_t                         genSize0; 
  uint64_t                         genSize ; 
  uint64_t                         genSizeMap ; 
#if USE_NEXT_OWNER
  uint64_t                        *ownersArray ; 
  uint64_t                         ownersArraySize ; 
#endif
  uint64_t                         upto[ismSTORE_GRANULE_POOLS_COUNT] ;
  uint64_t                       **pBitMaps ; 
  char                            *genData ; 
  ismStore_memDescriptor_t       **genDataMap[ismSTORE_GRANULE_POOLS_COUNT] ; 
  uint8_t                          isRecThere[ISM_STORE_NUM_REC_TYPES] ; 
#if USE_NEXT_OWNER
  ismStore_ownerByInd_t            refOwners[ISM_STORE_NUM_REC_TYPES] ; 
#endif
  ismStore_GenId_t                 genId ; 
  uint16_t                         genInd;
  uint16_t                         state ; //  see below
} ismStore_memGenInfo_t ; 

/*  bits of 'state'
  1 => read request made
  2 => read completed
  4 => inMem gen (bits 1 and 2 are also on)
  8 => ism_store_initGenMap() done (not really used)
 16 => ism_store_initRefGen done   (not really used)
 32 => ism_store_linkRefChanks done   (not really used)
 64 => gen in memory trace printed
128 => comp gen pending
256 => already processed
*/

extern ismStore_memGlobal_t   ismStore_memGlobal;
extern double viewTime;

typedef enum
{
  ISM_STORE_ITERTYPE_GENID,
  ISM_STORE_ITERTYPE_RECORD,
  ISM_STORE_ITERTYPE_REFCLI,
  ISM_STORE_ITERTYPE_REFQUE,
  ISM_STORE_ITERTYPE_STATE,
  ISM_STORE_ITERTYPE_OWNER
} ismStore_iterType ; 

typedef struct ismStore_Iterator_t
{
  uint64_t                        offset;
  uint64_t                        upto;
  uint64_t                        setOff;
  char                           *genData ; 
  char                           *bptr ; 
  char                           *eptr ; 
  ismStore_memSplitItem_t        *si ;
  ismStore_memRefGen_t           *rg ;
  ismStore_memReferenceContext_t *rCtx ;
#if USE_NEXT_OWNER
  ismStore_ownerByInd_t          *Owners ; 
#endif
  ismStore_iterType               type ; 
  ismStore_RecordType_t           recType ;
  ismStore_GenId_t                genId ; 
  ismStore_GenId_t                gid0 ; 
  ismStore_Handle_t               owner ; 
  ismStore_Handle_t               handle ; 
  ismStore_Handle_t               cache ;
  int                             isSi ; 
  int                             index ; 
  int                             blockSize ; 
  int                             poolId ; 
  int                             setLen ; 
  int                             setNew ; 
  int                             setMap ; 
  int                             refShift ; 
  int                             refCount ; 
} ismStore_Iterator_t ; 

typedef struct
{
  int nTypes ; 
  int f1st4gen ; 
  int rTypes[ISM_STORE_RECTYPE_MAXVAL] ; 
} RecTypes_t ; 

static int ism_store_initGenMap(ismStore_memGenInfo_t *gi, int withBits);
static int internal_readAhead(void);
static int32_t internal_memRecoveryUpdGeneration(ismStore_GenId_t genId, uint64_t **pBitMaps, uint64_t predictedSizeBytes);
static const char *recName(ismStore_RecordType_t type);

/*---------------------------------------------------------------------------*/

static RecTypes_t RT[1] ; 
static uint8_t T2T[ISM_STORE_RECTYPE_MAXVAL] ; 
static uint8_t hasProp[ISM_STORE_NUM_REC_TYPES] ; 
static ismStore_RecoveryParameters_t params[1] ; 
static uint64_t curMem ;
static ismStore_memGenInfo_t *allGens ; 
static int numGens, minGen, maxGen , gid1st, isOn, prevGens[3], curGens;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER ;
static pthread_cond_t  cond = PTHREAD_COND_INITIALIZER ;
#if USE_REC_TIME
static double recTime ; 
#endif
static double recTimes[10];
#if USE_NEXT_OWNER
static ismStore_ownerByInd_t newOwners[ISM_STORE_NUM_REC_TYPES];
static ismStore_ownerByInd_t prpOwners[ISM_STORE_NUM_REC_TYPES];
static uint64_t *ownersArray, ownersArraySize ; 
#endif

/*---------------------------------------------------------------------------*/

static void initRecTypes(void)
{
  memset(T2T,0,sizeof(T2T));
  RT->nTypes = 0 ; 
  T2T[ISM_STORE_RECTYPE_SERVER] = RT->nTypes++ ; 
  T2T[ISM_STORE_RECTYPE_CLIENT] = RT->nTypes++ ; 
  T2T[ISM_STORE_RECTYPE_QUEUE ] = RT->nTypes++ ; 
  T2T[ISM_STORE_RECTYPE_TOPIC ] = RT->nTypes++ ; 
  T2T[ISM_STORE_RECTYPE_SUBSC ] = RT->nTypes++ ; 
  T2T[ISM_STORE_RECTYPE_TRANS ] = RT->nTypes++ ; 
  T2T[ISM_STORE_RECTYPE_BMGR  ] = RT->nTypes++ ; 
  T2T[ISM_STORE_RECTYPE_REMSRV] = RT->nTypes++ ; 
  T2T[ISM_STORE_RECTYPE_MSG   ] = RT->nTypes++ ; 
  T2T[ISM_STORE_RECTYPE_PROP  ] = RT->nTypes++ ; 
  T2T[ISM_STORE_RECTYPE_CPROP ] = RT->nTypes++ ; 
  T2T[ISM_STORE_RECTYPE_QPROP ] = RT->nTypes++ ; 
  T2T[ISM_STORE_RECTYPE_TPROP ] = RT->nTypes++ ; 
  T2T[ISM_STORE_RECTYPE_SPROP ] = RT->nTypes++ ; 
  T2T[ISM_STORE_RECTYPE_BXR   ] = RT->nTypes++ ; 
  T2T[ISM_STORE_RECTYPE_RPROP ] = RT->nTypes++ ; 
  RT->nTypes = 0 ; 
  RT->rTypes[RT->nTypes++] = ISM_STORE_RECTYPE_SERVER ; 
  RT->rTypes[RT->nTypes++] = ISM_STORE_RECTYPE_CLIENT ; 
  RT->rTypes[RT->nTypes++] = ISM_STORE_RECTYPE_QUEUE  ; 
  RT->rTypes[RT->nTypes++] = ISM_STORE_RECTYPE_TOPIC  ; 
  RT->rTypes[RT->nTypes++] = ISM_STORE_RECTYPE_SUBSC  ; 
  RT->rTypes[RT->nTypes++] = ISM_STORE_RECTYPE_TRANS  ; 
  RT->rTypes[RT->nTypes++] = ISM_STORE_RECTYPE_BMGR   ; 
  RT->rTypes[RT->nTypes++] = ISM_STORE_RECTYPE_REMSRV ; 
  RT->f1st4gen = RT->nTypes ; 
  RT->rTypes[RT->nTypes++] = ISM_STORE_RECTYPE_MSG    ; 
  RT->rTypes[RT->nTypes++] = ISM_STORE_RECTYPE_PROP   ; 
  RT->rTypes[RT->nTypes++] = ISM_STORE_RECTYPE_CPROP  ; 
  RT->rTypes[RT->nTypes++] = ISM_STORE_RECTYPE_QPROP  ; 
  RT->rTypes[RT->nTypes++] = ISM_STORE_RECTYPE_TPROP  ; 
  RT->rTypes[RT->nTypes++] = ISM_STORE_RECTYPE_SPROP  ; 
  RT->rTypes[RT->nTypes++] = ISM_STORE_RECTYPE_BXR    ; 
  RT->rTypes[RT->nTypes++] = ISM_STORE_RECTYPE_RPROP  ; 

  memset(hasProp,0,sizeof(hasProp));
  hasProp[T2T[ISM_STORE_RECTYPE_CLIENT]] = 1 ; 
  hasProp[T2T[ISM_STORE_RECTYPE_QUEUE ]] = 1 ; 
  hasProp[T2T[ISM_STORE_RECTYPE_SUBSC ]] = 1 ; 
  hasProp[T2T[ISM_STORE_RECTYPE_REMSRV]] = 1 ; 
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static void ism_store_cmpDO(ismStore_GenId_t GenId, int32_t retcode, ismStore_DiskGenInfo_t *dgi, void *pContext)
{
}

/*---------------------------------------------------------------------------*/
static void ism_store_cmpCB(ismStore_GenId_t GenId, int32_t retcode, ismStore_DiskGenInfo_t *dgi, void *pContext)
{
  ismStore_memGenInfo_t *gi ; 
  int gid = GenId ; 

  if ( retcode != ISMRC_OK )
  {
    if ( retcode == StoreRC_Disk_TaskCancelled || retcode == StoreRC_Disk_TaskInterrupted )
    {
      TRACE(5,"Compaction task for genId %u has been cancelled or interrupted\n",GenId);
    }
    else
    {
      TRACE(1,"Compaction task for genId %u has failed with error code %d\n",GenId, retcode);
    }
    return; 
  }

  pthread_mutex_lock(&lock) ; 
  do
  {
    if ( gid < minGen || gid > maxGen )
    {
      TRACE(1,"Bad arguments: function %s, gid %d, minGen %d, maxGen %d\n",__FUNCTION__, gid, minGen, maxGen);
      break ; 
    }
      
    gi = allGens + (gid-minGen) ;
    if ( gi->genId != GenId || (gi->state&4) != 0 )
    {
      TRACE(1,"Bad arguments: function %s, gi->genId %d, GenId %d, gi->state %x\n", __FUNCTION__, gi->genId , GenId , gi->state);
      break ; 
    }
      
    if ( (gi->state&3) != 3 )
    {
      gi->genSize = dgi->DataLength ; 
    }
    else
      gi->genSize0= dgi->DataLength ; 
    internal_readAhead() ;
  } while(0) ; 
  pthread_mutex_unlock(&lock) ; 
}
/*---------------------------------------------------------------------------*/
static void ism_store_recCB(ismStore_GenId_t GenId, int32_t retcode, ismStore_DiskGenInfo_t *dummy, void *pContext)
{
  ismStore_memGenInfo_t *gi ; 
  int gid = GenId ; 

  pthread_mutex_lock(&lock) ; 
  do
  {
    if ( gid < minGen || gid > maxGen )
    {
      TRACE(1,"Bad arguments: function %s, gid %d, minGen %d, maxGen %d\n",__FUNCTION__, gid, minGen, maxGen);
      break ; 
    }
      
    gi = allGens + (gid-minGen) ;
    if ( gi->genId != GenId || (gi->state&7) != 1 )
    {
      TRACE(1,"Bad arguments: function %s, gi->genId %d, GenId %d, gi->state %x\n", __FUNCTION__, gi->genId , GenId , gi->state);
      break ; 
    }
  
    if ( retcode != ISMRC_OK )
    {
      if ( retcode == StoreRC_Disk_TaskCancelled || retcode == StoreRC_Disk_TaskInterrupted )
      {
        TRACE(5,"Read task for genId %u has been cancelled or interupted\n",GenId);
      }
      else
      {
        TRACE(1,"Read task for genId %u has failed with error code %d\n",GenId, retcode);
      }
      gi->state &= ~3; 
    }
    else
    {
      ism_store_initGenMap(gi,1) ;
      gi->useTime = su_sysTime() ; 
      if ( (gi->state&128) )
      {
        internal_memRecoveryUpdGeneration(gi->genId, gi->pBitMaps, 0);
        gi->state &= (~128) ; 
        gi->pBitMaps = NULL ; 
      }
      __sync_synchronize() ; 
      gi->state |= 2 ; 
    }
    pthread_cond_signal(&cond);
  } while(0) ; 
  pthread_mutex_unlock(&lock) ; 
}
/*---------------------------------------------------------------------------*/

static char * ism_store_getGenMem(size_t genSize, int force, int gid, int *ec)
{
  void *p=NULL ; 
  ismStore_memGenInfo_t *gi ; 
  int rc = ISMRC_OK ; 

  for(;;)
  {
    if ( curMem >= genSize )
    {
      if ( posix_memalign(&p,ismStore_memGlobal.DiskBlockSizeBytes, genSize) == 0 )
      {
        curMem -= genSize ;
        TRACE(5,"Recovery memory of size %lu has been allocated for genId %u , curMem= %lu\n",genSize,gid,curMem);
        break ; 
      }
      rc = ISMRC_AllocateError ;
      break ; 
    }
    if ( force )
    {
      double ot = su_sysTime() + 1e0 ; 
      int i,j,k ; 
      k = maxGen - minGen + 1 ; 
      for ( i=0,j=-1 ; i<k ; i++ )
      {
        gi = allGens+i ; 
        if ( gi->genId == gid ) continue ;
        if ( gi->genSizeMap > 0 && (gi->state&7) == 3 && gi->useTime < ot )
        {
          ot = gi->useTime ; 
          j = i ; 
        }
      }
      if ( j > -1 )
      {
        gi = allGens+j ; 
        ism_common_free_memaligned(ism_memory_store_misc,gi->genDataMap[0]) ;
        curMem += gi->genSizeMap ; 
        TRACE(5,"Recovery memory of size %lu has been taken from genId %u for genId %u, curMem= %lu\n",gi->genSizeMap,gi->genId,gid,curMem);
        gi->genDataMap[0] = NULL ; 
        gi->genSizeMap    = 0 ; 
        continue ;
      }
     #if USE_NEXT_OWNER
      for ( i=0,j=-1 ; i<k ; i++ )
      {
        gi = allGens+i ; 
        if ( gi->genId == gid ) continue ;
        if ( gi->ownersArraySize > 0 && (gi->state&7) == 3 && gi->useTime < ot )
        {
          ot = gi->useTime ; 
          j = i ; 
        }
      }
      if ( j > -1 )
      {
        gi = allGens+j ; 
        ism_common_free_memaligned(ism_memory_store_misc,gi->ownersArray) ;
        curMem += gi->ownersArraySize*sizeof(uint64_t) ; 
        TRACE(5,"Recovery memory of size %lu has been taken from genId %u for genId %u, curMem= %lu\n",gi->ownersArraySize*sizeof(uint64_t),gi->genId,gid,curMem);
        gi->ownersArray = NULL ;  
        gi->ownersArraySize = 0 ; 
        continue ;
      }
     #endif
      if ( force > 1 )
      {
        curMem += genSize ;
        TRACE(5,"Recovery memory of size %lu has been overdrawn for genId %u, curMem= %lu\n",genSize,gid,curMem);
        continue ; 
      }
      for ( i=0,j=-1 ; i<k ; i++ )
      {
        gi = allGens+i ; 
        if ( gi->genId == gid ) continue ;
        if ( (gi->state&7) == 3 && gi->useTime < ot )
        {
          ot = gi->useTime ; 
          j = i ; 
        }
      }
      if ( j > -1 )
      {
        gi = allGens+j ; 
        ism_common_free(ism_memory_store_misc,gi->genData) ; 
        curMem += gi->genSize ; 
        TRACE(5,"Recovery memory of size %lu has been taken from genId %u for genId %u, curMem= %lu\n",gi->genSize,gi->genId,gid,curMem);
        gi->genData = NULL ; 
        gi->state &= (~(64|3)) ; 
        continue ;
      }
     #if 0
      if ( sleep )
      {
        TRACE(5,"Waiting 5 second for Recovery memory of size %lu to become available for genId %u, curMem= %lu\n",genSize,gid,curMem);
        su_sleep(5,SU_SEC) ; 
        sleep-- ; 
        continue ; 
      }
     #endif
      rc = ISMRC_Error ;
    }
    break ; 
  }
  if ( ec ) *ec = rc ; 
  return p ;
}     

/*------------------------------------------------------*/
static inline ismStore_memDescriptor_t *off2desc(ismStore_memGenInfo_t *gi, uint64_t offset)
{
  int i,j;
  ismStore_memGenHeader_t *pGenHeader = (ismStore_memGenHeader_t *)gi->genData ;
  if ( offset < gi->upto[0] )
    i = 0 ; 
  else
  {
    int l = pGenHeader->PoolsCount<=ismSTORE_GRANULE_POOLS_COUNT ? pGenHeader->PoolsCount : ismSTORE_GRANULE_POOLS_COUNT ; 
    for ( i=1 ; i < l ; i++ )
      if ( offset < gi->upto[i] ) break ; 
    if ( i >= l )
      return NULL ; 
  }
  j = (offset - pGenHeader->GranulePool[i].Offset) / pGenHeader->GranulePool[i].GranuleSizeBytes ; 
  if (! gi->genDataMap[i][j] )
  {
    ismStore_memDescriptor_t *desc ; 
    ismStore_memGranulePool_t *pPool;
    char *bptr, *eptr ; 
    size_t DS, off ; 

    TRACE(1,"off2desc: !!! gid=%u, off=%lu, upto=%lu, %lu, i,j=%d, %d, cs=%lu\n",gi->genId,offset,gi->upto[0],gi->upto[1],i,j,pGenHeader->CompactSizeBytes);
    if ( SHOULD_TRACE(9) )
    {
      DS = pGenHeader->DescriptorStructSize ; 
      bptr = (char *)pGenHeader + pGenHeader->GranulePool[0].Offset ; 
      eptr = (char *)pGenHeader + pGenHeader->CompactSizeBytes ; 
      while ( bptr < eptr )
      {
        desc = (ismStore_memDescriptor_t *)bptr ; 
        i = desc->PoolId ; 
        j = desc->GranuleIndex ; 
        pPool = &pGenHeader->GranulePool[desc->PoolId] ; 
        off = pPool->Offset + (size_t)desc->GranuleIndex * pPool->GranuleSizeBytes ; 
        TRACE(9,"off2desc: >>> off=%lx, i,j=%d, %d, desc=%p %p, type=%x \n",off,i,j,desc,gi->genDataMap[i][j],desc->DataType);
        bptr += ALIGNED(DS+desc->DataLength) ; 
      }
    }
    return NULL ; 
  }
  return gi->genDataMap[i][j] ; 
}
/*------------------------------------------------------*/
static int ism_store_getGenMap(ismStore_GenId_t gid)
{
  int rc, i; 
  register size_t DS ; 
  size_t memSize ; 
  ismStore_memGenInfo_t *gi ; 
  ismStore_memDescriptor_t *desc ; 
  ismStore_memGenHeader_t *pGenHeader ;
  char *bptr, *eptr, *mapData ; 

  gi = allGens + (gid-minGen) ;
  if ( gi->genDataMap[0] )
    return ISMRC_OK ; 
  pGenHeader = (ismStore_memGenHeader_t *)gi->genData ;
  DS = pGenHeader->DescriptorStructSize ; 
  for ( i=0, memSize=0 ; i < pGenHeader->PoolsCount ; i++ )
  {
    gi->upto[i] = pGenHeader->GranulePool[i].Offset + pGenHeader->GranulePool[i].MaxMemSizeBytes ; 
    memSize += pGenHeader->GranulePool[i].MaxMemSizeBytes / pGenHeader->GranulePool[i].GranuleSizeBytes ; 
  }
  memSize *= sizeof(void *) ; 
  mapData = ism_store_getGenMem(memSize, 2, gid, &rc) ; 
  if ( !mapData )
  {
    return ISMRC_AllocateError ;
  }
  gi->genSizeMap = memSize ; 
  for ( i=0, memSize=0 ; i < pGenHeader->PoolsCount ; i++ )
  {
    gi->genDataMap[i] = (ismStore_memDescriptor_t **)(mapData + memSize) ; 
    memSize += (pGenHeader->GranulePool[i].MaxMemSizeBytes / pGenHeader->GranulePool[i].GranuleSizeBytes) * sizeof(void *) ; 
  }

  memset(mapData, 0, memSize) ; 
  bptr = gi->genData + pGenHeader->GranulePool[0].Offset ; 
  eptr = gi->genData + pGenHeader->CompactSizeBytes ;
  while ( bptr < eptr )
  {
    desc = (ismStore_memDescriptor_t *)bptr ; 
    gi->genDataMap[desc->PoolId][desc->GranuleIndex] = desc ; 
    bptr += ALIGNED(DS+desc->DataLength) ;
  }
  return ISMRC_OK ; 
}

/*---------------------------------------------------------------------------*/
static char * ism_store_getGen(ismStore_GenId_t gid, int *ec)
{
  int rc ; 
  ismStore_memGenInfo_t *gi ; 

  if ( gid < minGen || gid > maxGen )
  {
    TRACE(1,"Bad arguments: function %s, gid %d, minGen %d, maxGen %d\n",__FUNCTION__, gid, minGen, maxGen);
    ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "GenId");
    *ec = ISMRC_ArgNotValid ;
    return NULL ; 
  }
    
  gi = allGens + (gid-minGen) ;
  if ( gi->genSize == 0 || gi->genId != gid )
  {
    TRACE(1,"Bad arguments: function %s, gid %d, gi->genSize %lu, gi->genId %d\n",__FUNCTION__, gid, gi->genSize, gi->genId);
    ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "GenId");
    *ec = ISMRC_ArgNotValid ;
    return NULL ; 
  }

  if ( gi->state&2 ) 
  {
    if (!(gi->state&64) )
    {
      TRACE(5,"Generation %u is already in memory!\n",gid);
      gi->state |= 64 ; 
    }
    return gi->genData ; 
  }

  pthread_mutex_lock(&lock) ; 
  rc = ISMRC_OK ;
  do
  {
    if ( gi->state&2 ) 
    {
      if (!(gi->state&64) )
      {
        TRACE(5,"Generation %u is already in memory!\n",gid);
        gi->state |= 64 ; 
      }
      break ; 
    }
    if (!(gi->state&1) ) 
    {
      ismStore_DiskBufferParams_t *bp ;
      ismStore_DiskTaskParams_t dtp[1] ; 
      if ( gi->genSize < gi->genSize0 ) 
           gi->genSize = gi->genSize0 ; 
      gi->genData = ism_store_getGenMem(gi->genSize, 1, gid, &rc) ; 
      if (!gi->genData )
        break ; 

      gi->state |= 1 ; 
      memset(dtp,0,sizeof(dtp)) ; 
      dtp->fCancelOnTerm = 1 ; 
      dtp->Priority      = 1 ; 
      dtp->GenId         = gid ; 
      dtp->Callback      = ism_store_recCB ; 
      dtp->pContext      = gi ; 
      bp = dtp->BufferParams ; 
      bp->pBuffer      = gi->genData ; 
      bp->BufferLength = gi->genSize ; 
      if ( (rc = ism_storeDisk_readGeneration(dtp)) ) 
      {
        if ( rc == StoreRC_BadParameter ) rc = ISMRC_ArgNotValid ; 
        if ( rc == StoreRC_Disk_IsNotOn ) rc = ISMRC_Error ; 
        if ( rc == StoreRC_AllocateError) rc = ISMRC_AllocateError ;
        break ; 
      }
    }
    {
      double td = su_sysTime() ;
      while ( (gi->state&3) == 1 ) /* BEAM suppression: infinite loop */
        pthread_cond_wait(&cond,&lock) ; 
      recTimes[7] += su_sysTime() - td ; 
    }
    if ((gi->state&3) != 3 )
      rc = ISMRC_Error ;
    else
    if (!(gi->state&64) )
    {
      TRACE(5,"Generation %u has been read from disk!\n",gid);
      gi->state |= 64 ; 
    }
  } while (0) ; 
  gi->useTime = su_sysTime() ; 
  pthread_mutex_unlock(&lock) ; 
  if ( rc ) 
  {
    *ec = rc ; 
    return NULL ; 
  }
  return gi->genData ; 
}
/*---------------------------------------------------------------------------*/
static int ism_store_isGenIn(ismStore_GenId_t gid)
{
  ismStore_memGenInfo_t *gi ; 

  if ( gid < minGen || gid > maxGen )
  {
    return -1 ; 
  }
    
  gi = allGens + (gid-minGen) ;
  if ( gi->genSize == 0 || gi->genId != gid )
  {
    return -1 ; 
  }

  return (gi->state&3) ;  
}
/*---------------------------------------------------------------------------*/
#if 0
static int qcmp(const void *a, const void *b)
{
  const ismStore_memReferenceChunk_t *cha, *chb ; 
  cha = *(ismStore_memReferenceChunk_t * const *)a ; 
  chb = *(ismStore_memReferenceChunk_t * const *)b ; 
  return (int)(cha->BaseOrderId - chb->BaseOrderId) ; 
}
#endif
static int qcmp2(const void *a, const void *b)
{
  int64_t dif ; 
  const ismStore_memReferenceChunk_t *cha, *chb ; 
  cha = *(ismStore_memReferenceChunk_t * const *)a ; 
  chb = *(ismStore_memReferenceChunk_t * const *)b ; 
  if (!(dif = cha->OwnerHandle - chb->OwnerHandle) )
        dif = cha->BaseOrderId - chb->BaseOrderId ; 
  if ( dif < 0 ) return -1 ; 
  if ( dif > 0 ) return  1 ; 
  return 0 ; 
}
#if USE_NEXT_OWNER
static int qcmp3(const void *a, const void *b)
{
  uint64_t ia, ib;
  ia = *(uint64_t *)a ; 
  ib = *(uint64_t *)b ; 
  if ( ia < ib ) return -1 ; 
  if ( ia > ib ) return  1 ; 
  return 0 ; 
}
#endif
/*---------------------------------------------------------------------------*/

static int ism_store_linkRefChanks(ismStore_memGenHeader_t *pGenHeader)
{
  int i ; 
  register size_t DS;
  uint64_t offset, upto, blocksize ; 
  ismStore_memMgmtHeader_t *pMgmHeader;
  ismStore_memDescriptor_t *desc, *desc2, *head, *tail;
  ismStore_Handle_t handle ;
  ismStore_memSplitItem_t *si ;
  ismStore_GenId_t gid = pGenHeader->GenId ;
  size_t len ;

  if ( pGenHeader->CompactSizeBytes )
  {
    TRACE(1,"ism_store_linkRefChanks can not operate on a compacted generation ; genId=%u\n",gid);
    return ISMRC_ArgNotValid ;
  }
  DS = pGenHeader->DescriptorStructSize ; 
  pMgmHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress;
  offset = upto = i = blocksize = len = 0 ; 
  head = tail = NULL ; 
  for(;;)
  {
    if ( offset >= upto )
    {
      if ( pGenHeader->PoolsCount > i && offset <= pGenHeader->GranulePool[i].Offset )
      {
        blocksize = pGenHeader->GranulePool[i].GranuleSizeBytes ; 
        offset    = pGenHeader->GranulePool[i].Offset ; 
        upto      = offset + pGenHeader->GranulePool[i].MaxMemSizeBytes ; 
        i++ ; 
      }
      else
        break ; 
    }
    desc = (ismStore_memDescriptor_t *)((uintptr_t)pGenHeader + offset) ;
    if ( (desc->DataType&(~ismSTORE_DATATYPE_NOT_PRIMARY)) == ismSTORE_DATATYPE_REFERENCES )
    {
      ismStore_memReferenceChunk_t *ch ;
      ch = (ismStore_memReferenceChunk_t *)((uintptr_t)desc+DS) ;
      desc2 = (ismStore_memDescriptor_t *)(ismStore_memGlobal.pStoreBaseAddress + ismSTORE_EXTRACT_OFFSET(ch->OwnerHandle)) ; 
      handle = ismSTORE_BUILD_HANDLE(gid,offset) ; 
      if ( ismSTORE_IS_SPLITITEM(desc2->DataType) )
      {
        si = (ismStore_memSplitItem_t *)((uintptr_t)desc2+pMgmHeader->DescriptorStructSize) ;
        if ( si->Version == ch->OwnerVersion )
        {
          int j;
          for ( j=0 ; j<ch->ReferenceCount && !ch->References[j].RefHandle ; j++ ) ; // empty body
          if ( j<ch->ReferenceCount )
          {
            if ( tail )
              tail->NextHandle = (ismStore_Handle_t)desc ; 
            else
              head = desc ; 
            tail = desc ; 
            len++ ;
          }
          else
          {
            TRACE(8,"ism_store_linkRefChanks: An Empty ReferenceChunk: chunk_handle=%lx , chunk_version=%u , owner_handle=%lx, owner_version=%u\n",
                  handle,ch->OwnerVersion,ch->OwnerHandle,si->Version);
            desc->DataType = ismSTORE_DATATYPE_FREE_GRANULE ; 
            desc->NextHandle = ismSTORE_NULL_HANDLE ; 
          } 
        }
        else
        {
          TRACE(8,"ism_store_linkRefChanks: An Orphan ReferenceChunk: chunk_handle=%lx , chunk_version=%u , owner_handle=%lx, owner_version=%u\n",
                handle,ch->OwnerVersion,ch->OwnerHandle,si->Version);
          desc->DataType = ismSTORE_DATATYPE_FREE_GRANULE ; 
          desc->NextHandle = ismSTORE_NULL_HANDLE ; 
        }
      }
      else
      {
        TRACE(8,"ism_store_linkRefChanks: An Orphan ReferenceChunk: chunk_handle=%lx , owner_handle=%lx, owner_type=%x\n",
                 handle,ch->OwnerHandle,desc2->DataType);
        desc->DataType = ismSTORE_DATATYPE_FREE_GRANULE ; 
        desc->NextHandle = ismSTORE_NULL_HANDLE ; 
      }
    }
    offset += blocksize ; 
  }
  if ( len )
  {
    ismStore_memReferenceChunk_t *ch ; 
    ismStore_memReferenceChunk_t **vec=NULL;
    offset = i = blocksize = 0 ; 
    if (!(vec = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,38),len*sizeof(void *))) )
    {
      TRACE(1,"%s failed to allocate memory of %lu bytes\n", __FUNCTION__, len*sizeof(void *));
      return ISMRC_AllocateError ;
    }
    tail->NextHandle = 0 ; 
    for ( i=0, desc=head ; desc ; desc = (ismStore_memDescriptor_t *)desc->NextHandle )
    {
      ch = (ismStore_memReferenceChunk_t *)((uintptr_t)desc+DS) ;
      vec[i++] = ch ; 
    }

    qsort(vec, len, sizeof(void *),qcmp2);
    for ( i=0, handle=0, desc2=NULL ; i<len ; i++ )
    {
      ch = vec[i] ; 
      desc = (ismStore_memDescriptor_t *)((uintptr_t)ch-DS) ;
      offset = (uintptr_t)desc - (uintptr_t)pGenHeader ; 
      if ( handle == ch->OwnerHandle )
      {
        desc->DataType |= (ismSTORE_DATATYPE_NOT_PRIMARY) ;
        if ( desc2 )  // just for BEAM
        desc2->NextHandle = ismSTORE_BUILD_HANDLE(gid,offset) ; 
      }
      else
      {
        desc->DataType &= (~ismSTORE_DATATYPE_NOT_PRIMARY) ;
        handle = ch->OwnerHandle ;
        if ( desc2 )
          desc2->NextHandle = 0 ; 
      }
      desc2 = desc ; 
    }
    if ( desc2 )
      desc2->NextHandle = 0 ; 
    ism_common_free(ism_memory_store_misc,vec) ; 
  }
  return ISMRC_OK ;
}

/*---------------------------------------------------------------------------*/

int32_t ism_store_memRecoveryLinkRefChanks(ismStore_memGenHeader_t *pGenHeader)
{
  int rc ; 
  pthread_mutex_lock(&lock) ; 
  rc = ism_store_linkRefChanks(pGenHeader) ; 
  pthread_mutex_unlock(&lock) ; 
  return rc ; 
}

/*---------------------------------------------------------------------------*/
static int ism_store_addRefGen(ismStore_memGenInfo_t *gi, ismStore_memReferenceContext_t *rCtx, uint64_t mnOid, uint64_t mxOid, uint64_t mxOidU,
                               ismStore_Handle_t head, ismStore_Handle_t tail, uint32_t numChunks, ismStore_Handle_t owner)
{
  ismStore_memRefGen_t *rg, *prg, *nrg;
  ismStore_GenId_t ngid ; 
  ismStore_memGenInfo_t *ngi ; 

//if ( !(rg = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,40),sizeof(ismStore_memRefGen_t))) )
  ismSTORE_MUTEX_LOCK(rCtx->pMutex);
  rg = ism_store_memAllocateRefGen(rCtx) ; 
  if ( rCtx->HighestOrderId < mxOidU )
       rCtx->HighestOrderId = mxOidU ;
  ismSTORE_MUTEX_UNLOCK(rCtx->pMutex);
  if ( !rg )
  {
    TRACE(1,"%s failed to allocate memory of %lu bytes\n", __FUNCTION__, sizeof(ismStore_memRefGen_t));
    return ISMRC_AllocateError ;
  }
  memset(rg, 0, sizeof(ismStore_memRefGen_t)) ;
  rg->LowestOrderId  = mnOid ; 
  rg->HighestOrderId = mxOid ; 
  rg->hReferenceHead = head ; 
  rg->hReferenceTail = tail ; 
  rg->numChunks      = numChunks*100 ; 
  if ( rCtx->pRefGenLast )
  {
    nrg = rCtx->pRefGenLast ; 
    ngid = ismSTORE_EXTRACT_GENID(nrg->hReferenceHead) ;
    ngi  = allGens + (ngid-minGen) ; 
    if ( gi->genInd < ngi->genInd )
    {
      prg = NULL ; 
      nrg = rCtx->pRefGenHead ;
    }
    else
    {
      prg = nrg ; 
      nrg = nrg->Next ; 
    }
  }
  else
  {
    prg = NULL ; 
    nrg = rCtx->pRefGenHead ;
  }
  for ( ; nrg ; prg=nrg, nrg=nrg->Next )
  {
    ngid = ismSTORE_EXTRACT_GENID(nrg->hReferenceHead) ;
    ngi  = allGens + (ngid-minGen) ; 
    if ( gi->genInd < ngi->genInd )
      break ; 
  }
  rg->Next = nrg ; 
  if (!prg )
    rCtx->pRefGenHead = rg ; 
  else
    prg->Next = rg ; 
  TRACE(9,"Adding ismStore_memRefGen_t (%p) to owner 0x%lx, mnOid=%lu, mxOid=%lu, head=0x%lx, tail=0x%lx\n",rg,owner,mnOid,mxOid,head,tail);
  return ISMRC_OK ;
}
/*---------------------------------------------------------------------------*/
static void ism_store_removeRefGen(ismStore_IteratorHandle  pIter)
{
  ismStore_memRefGen_t *prg, *rg ;
  //ism_store_memPrintRefGen(pIter->rCtx, pIter->rg) ; 
  for (prg=NULL, rg=pIter->rCtx->pRefGenHead ; rg ; prg=rg, rg=rg->Next )
  {
    if ( rg == pIter->rg )
    {
      if ( prg )
      {
        pIter->rCtx->pRefGenLast = prg ;
        prg->Next = rg->Next ; 
      }
      else
      {
        pIter->rCtx->pRefGenLast = NULL ;
        pIter->rCtx->pRefGenHead = rg->Next ; 
      }
      TRACE(9,"Removing ismStore_memRefGen_t (%p) from owner 0x%lx, mnOid=%lu, mxOid=%lu, head=0x%lx, tail=0x%lx\n",rg,pIter->rCtx->OwnerHandle,rg->LowestOrderId,rg->HighestOrderId,rg->hReferenceHead,rg->hReferenceTail);
      ismSTORE_MUTEX_LOCK(pIter->rCtx->pMutex); 
      ism_store_memFreeRefGen(pIter->rCtx,rg);
      ismSTORE_MUTEX_UNLOCK(pIter->rCtx->pMutex);
      break ; 
    }
  }
}
/*---------------------------------------------------------------------------*/
static int ism_store_initRefGen(ismStore_memGenHeader_t *pGenHeader)
{
  int i, rc ; 
  register size_t DS ; 
  uint64_t offset, upto, blocksize, mnOid, mxOid, mxOidU ; 
  ismStore_memMgmtHeader_t *pMgmHeader;
  ismStore_Handle_t handle, head, tail, owner;
  ismStore_GenId_t gid = pGenHeader->GenId ;
  ismStore_memGenInfo_t *gi ;
  uint32_t nc ; 

  gi = allGens + (gid-minGen) ;
  if ( gi->state & 16 ) 
    return ISMRC_OK ;
  DS = pGenHeader->DescriptorStructSize ; 
  pMgmHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress;
  if ( pGenHeader->CompactSizeBytes )
  {
    ismStore_memDescriptor_t *desc ;
    ismStore_memGranulePool_t *pPool;
    char *bptr, *eptr ; 
  
    bptr = (char *)pGenHeader + pGenHeader->GranulePool[0].Offset ; 
    eptr = (char *)pGenHeader + pGenHeader->CompactSizeBytes ; 
    while ( bptr < eptr )
    {
      desc = (ismStore_memDescriptor_t *)bptr ; 
      bptr += ALIGNED(DS+desc->DataLength) ; 
      pPool = &pGenHeader->GranulePool[desc->PoolId] ; 
      offset = pPool->Offset + (size_t)desc->GranuleIndex * pPool->GranuleSizeBytes ; 
      if ( desc->DataType >= ISM_STORE_RECTYPE_SERVER &&
           desc->DataType <  ISM_STORE_RECTYPE_MAXVAL )
        gi->isRecThere[T2T[desc->DataType]] = 1 ; 
      else
      if ( desc->DataType == ismSTORE_DATATYPE_REFERENCES )
      {
        ismStore_memDescriptor_t *desc2;
        ismStore_memReferenceContext_t *rCtx ;
        ismStore_memSplitItem_t *si ;
        ismStore_memReferenceChunk_t *ch = (ismStore_memReferenceChunk_t *)((uintptr_t)desc+DS) ;
        owner = ch->OwnerHandle ; 
        handle = ismSTORE_BUILD_HANDLE(gid,offset) ; 
        head = handle ; 
        tail = handle ; 
        mnOid = ch->BaseOrderId ; 
        mxOid = ch->BaseOrderId + ch->ReferenceCount ; 
        desc2 = (ismStore_memDescriptor_t *)(ismStore_memGlobal.pStoreBaseAddress + ismSTORE_EXTRACT_OFFSET(owner)) ;
        if ( ismSTORE_IS_SPLITITEM(desc2->DataType) )
        {
          si = (ismStore_memSplitItem_t *)((uintptr_t)desc2+pMgmHeader->DescriptorStructSize) ;
          if ( si->Version == ch->OwnerVersion )
          {
            if ( !(rCtx = (ismStore_memReferenceContext_t *)si->pRefCtxt) )
            {
              TRACE(1,"Bad arguments: function %s, si->pRefCtxt NULL!\n", __FUNCTION__);
              return ISMRC_Error ;
            }
            nc = 1 ; 
            while ( bptr < eptr && ismSTORE_EXTRACT_OFFSET(desc->NextHandle) )
            {
              desc = (ismStore_memDescriptor_t *)bptr ; 
              bptr += ALIGNED(DS+desc->DataLength) ; 
              offset = pPool->Offset + (size_t)desc->GranuleIndex * pPool->GranuleSizeBytes ; 
              handle = ismSTORE_BUILD_HANDLE(gid,offset) ; 
              tail = handle ; 
              ch = (ismStore_memReferenceChunk_t *)((uintptr_t)desc+DS) ;
              if ( mxOid < ch->BaseOrderId + ch->ReferenceCount ) 
                mxOid = ch->BaseOrderId + ch->ReferenceCount ;
              nc++ ; 
            }
            if ( mxOid > si->MinActiveOrderId )
            {
              uint64_t mxCount = (pPool->GranuleSizeBytes - DS - offsetof(ismStore_memReferenceChunk_t, References)) / sizeof(ismStore_memReference_t) ; 
              mxOidU = (mxOid + mxCount - 1)/ mxCount * mxCount ; 
              if ( (rc = ism_store_addRefGen(gi, rCtx, mnOid, mxOid-1, mxOidU, head, tail, nc, owner)) != ISMRC_OK )
                return rc ; 
            }
          }
        }
      }
    }
  }
  else
  {
    ismStore_memDescriptor_t *desc ;
    offset = upto = i = blocksize = 0 ; 
    if ( (gi->state&4) && !(gi->state&32) )
    {
      if ( (rc = ism_store_linkRefChanks(pGenHeader)) != ISMRC_OK )
        return rc ; 
      gi->state |= 32 ; 
    }
    for(;;)
    {
      if ( offset >= upto )
      {
        if ( pGenHeader->PoolsCount > i && offset <= pGenHeader->GranulePool[i].Offset )
        {
          blocksize = pGenHeader->GranulePool[i].GranuleSizeBytes ; 
          offset    = pGenHeader->GranulePool[i].Offset ; 
          upto      = offset + pGenHeader->GranulePool[i].MaxMemSizeBytes ; 
          i++ ; 
        }
        else
          break ; 
      }
      desc = (ismStore_memDescriptor_t *)((uintptr_t)pGenHeader + offset) ;
      if ( desc->DataType >= ISM_STORE_RECTYPE_SERVER &&
           desc->DataType <  ISM_STORE_RECTYPE_MAXVAL )
        gi->isRecThere[T2T[desc->DataType]] = 1 ; 
      else
      if ( desc->DataType == ismSTORE_DATATYPE_REFERENCES )
      {
        ismStore_memDescriptor_t *desc2;
        ismStore_memReferenceContext_t *rCtx ;
        ismStore_memSplitItem_t *si ;
        ismStore_memReferenceChunk_t *ch = (ismStore_memReferenceChunk_t *)((uintptr_t)desc+DS) ;
        owner = ch->OwnerHandle ; 
        handle = ismSTORE_BUILD_HANDLE(gid,offset) ; 
        head = handle ; 
        tail = handle ; 
        mnOid = ch->BaseOrderId ; 
        mxOid = ch->BaseOrderId + ch->ReferenceCount ; 
        desc2 = (ismStore_memDescriptor_t *)(ismStore_memGlobal.pStoreBaseAddress + ismSTORE_EXTRACT_OFFSET(owner)) ;
        if ( ismSTORE_IS_SPLITITEM(desc2->DataType) )
        {
          si = (ismStore_memSplitItem_t *)((uintptr_t)desc2+pMgmHeader->DescriptorStructSize) ;
          if ( si->Version == ch->OwnerVersion )
          {
            if ( !(rCtx = (ismStore_memReferenceContext_t *)si->pRefCtxt) )
            {
              TRACE(1,"Bad arguments: function %s, si->pRefCtxt NULL!\n", __FUNCTION__);
              return ISMRC_Error ;
            }
            nc = 1 ; 
            for ( handle=desc->NextHandle ; handle ; handle=desc->NextHandle )
            {
              desc = (ismStore_memDescriptor_t *)((uintptr_t)pGenHeader + ismSTORE_EXTRACT_OFFSET(handle)) ;
              ch = (ismStore_memReferenceChunk_t *)((uintptr_t)desc+DS) ;
              tail = handle ; 
              mxOid = ch->BaseOrderId + ch->ReferenceCount ; 
              nc++ ; 
            }
            if ( mxOid > si->MinActiveOrderId )
            {
              uint64_t mxCount = (blocksize - DS - offsetof(ismStore_memReferenceChunk_t, References)) / sizeof(ismStore_memReference_t) ; 
              mxOidU = (mxOid + mxCount - 1)/ mxCount * mxCount ; 
              if ( (rc = ism_store_addRefGen(gi, rCtx, mnOid, mxOid-1, mxOidU, head, tail, nc, owner)) != ISMRC_OK )
                return rc ; 
            }
          }
        }
      }
      offset += blocksize ; 
    }
  }
  gi->state |= 16 ; 
  return ISMRC_OK ;
}
/*---------------------------------------------------------------------------*/

static int ism_store_initGenMap(ismStore_memGenInfo_t *gi, int withBits)
{
  int gid ; 
  ismStore_memGenMap_t *gm;
  int iok=0 , rc ; 
  ismStore_memGenHeader_t *pGenHeader ; 

  if ( gi->state & 8 ) 
    return ISMRC_OK ;
  gid = gi->genId ; 
  pGenHeader = (ismStore_memGenHeader_t *)gi->genData ; 
  rc = ISMRC_OK ;
  do
  {
    if ( pGenHeader && isOn > 1 )
    {
      if ( !(gm = ismStore_memGlobal.pGensMap[gid]) )
      {
        rc = ISMRC_AllocateError ;
        if ( !(gm = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,41),sizeof(ismStore_memGenMap_t))) ) break ;
        iok++ ; 
        memset(gm,0,sizeof(ismStore_memGenMap_t));
        rc = ISMRC_Error ;
        if ( pthread_mutex_init(&gm->Mutex,NULL) ) break ; 
        iok++ ; 
        if ( pthread_cond_init(&gm->Cond,NULL) ) break ; 
        iok++ ; 
        if (!(gi->state&4) )
          gm->DiskFileSize = gi->genSize ; 
        ismStore_memGlobal.pGensMap[gid] = gm ;
        ismStore_memGlobal.GensMapCount++ ;
      }
      gm->MemSizeBytes     = pGenHeader->MemSizeBytes;
   // if ( withBits && !gm->GranulesMap->pBitMap[ismSTORE_BITMAP_LIVE_IDX] )
      {
        int i ; 
        size_t l;
        ismStore_memGranulesMap_t *grm;
        ismStore_memGranulePool_t *pPool;
      
        rc = ISMRC_AllocateError ;
        for ( i=0 ; i<pGenHeader->PoolsCount ; i++ )
        {
          pPool = &pGenHeader->GranulePool[i] ; 
          grm = &gm->GranulesMap[i] ; 
          grm->Offset = pPool->Offset ; 
          grm->Last = grm->Offset + pPool->MaxMemSizeBytes ; 
          grm->BitMapSize = (pPool->MaxMemSizeBytes / pPool->GranuleSizeBytes / 64 + 1) ; 
          l = grm->BitMapSize * sizeof(uint64_t) ; 
          grm->GranuleSizeBytes = pPool->GranuleSizeBytes ; 
          if ( withBits && !grm->pBitMap[ismSTORE_BITMAP_LIVE_IDX] )
          {
            if (!(grm->pBitMap[ismSTORE_BITMAP_LIVE_IDX] = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,42),l)) ) break ; 
            memset(grm->pBitMap[ismSTORE_BITMAP_LIVE_IDX],0,l) ; 
            gm->GranulesMapCount++ ; 
          }
        }
        if ( i<pGenHeader->PoolsCount ) break ; 
      }
      gi->state |=  8 ; 
      rc = ism_store_initRefGen(pGenHeader) ;
      if ( rc != ISMRC_OK ) break ;
    }
    rc = ISMRC_OK ;
  } while(0) ; 
  if ( rc != ISMRC_OK )
  {
    if ( iok > 2 )
      pthread_cond_destroy(&gm->Cond) ; 
    if ( iok > 1 )
      pthread_mutex_destroy(&gm->Mutex); 
    if ( iok > 0 )
    {
      int i ; 
      ismStore_memGranulesMap_t *grm;
      for ( i=0 ; i<pGenHeader->PoolsCount ; i++ )
      {
        grm = &gm->GranulesMap[i] ; 
        if ( grm->pBitMap[ismSTORE_BITMAP_LIVE_IDX] ) ism_common_free(ism_memory_store_misc,grm->pBitMap[ismSTORE_BITMAP_LIVE_IDX]) ; 
      }
      ism_common_free(ism_memory_store_misc,gm) ; 
      ismStore_memGlobal.pGensMap[gid] = NULL ; 
      ismStore_memGlobal.GensMapCount-- ;
    }
  }
  return rc ; 
}


/*---------------------------------------------------------------------------*/

static int32_t internal_memRecoveryInit(ismStore_RecoveryParameters_t *pRecoveryParams)
{
  int i  ; 

  if ( isOn ) return ISMRC_OK ;

 #if USE_REC_TIME
  recTime = 0e0 ; 
 #endif
 
  if ( !pRecoveryParams ) return ISMRC_ArgNotValid ; 
  memcpy(params, pRecoveryParams, sizeof(ismStore_RecoveryParameters_t)) ; 
  curMem = params->MaxMemoryBytes ; 
  TRACE(5,"Recovery memory has been set to MaxMemoryBytes, curMem= %lu\n",curMem);
  initRecTypes() ; 

  minGen = 1 ; 
  maxGen = 8 ; 
  i = maxGen - minGen + 1 ; 
  allGens = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,45),i*sizeof(ismStore_memGenInfo_t)) ; 
  if ( !allGens )
    return ISMRC_AllocateError ;
  memset(allGens,0,i*sizeof(ismStore_memGenInfo_t)) ; 

  isOn = 1 ; 
  viewTime = su_sysTime() ; 
  return ISMRC_OK ;
}
int32_t ism_store_memRecoveryInit(ismStore_RecoveryParameters_t *pRecoveryParams)
{
  int rc;

  pthread_mutex_lock(&lock) ; 
  rc = internal_memRecoveryInit(pRecoveryParams) ; 
  pthread_mutex_unlock(&lock) ; 
  return rc ; 
}
int32_t ism_store_memRecoveryUpdParams(ismStore_RecoveryParameters_t *pRecoveryParams)
{
  pthread_mutex_lock(&lock) ; 
  memcpy(params, pRecoveryParams, sizeof(ismStore_RecoveryParameters_t)) ; 
  curMem = params->MaxMemoryBytes ; 
  TRACE(5,"Recovery memory has been set to MaxMemoryBytes, curMem= %lu\n",curMem);
  pthread_mutex_unlock(&lock) ; 
  return ISMRC_OK;
}
/*---------------------------------------------------------------------------*/

static int32_t extend_allGens(int genId)
{
  void *tmp ; 
  size_t os, ns ; 

  os = (maxGen - minGen + 1) * sizeof(ismStore_memGenInfo_t) ; 
  if ( genId < minGen )
  {
    ns = (maxGen - genId  + 1) * sizeof(ismStore_memGenInfo_t) ; 
    tmp = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,46),ns) ; 
    if ( !tmp )
      return ISMRC_AllocateError ;
    memset(tmp, 0, ns) ; 
    memcpy(tmp+(ns-os), allGens, os) ; 
    ism_common_free(ism_memory_store_misc,allGens) ; 
    allGens = tmp ; 
    minGen = genId ; 
  }
  else
  if ( genId > maxGen )
  {
    ns = (genId  - minGen + 1) * sizeof(ismStore_memGenInfo_t) ; 
    tmp = ism_common_realloc(ISM_MEM_PROBE(ism_memory_store_misc,48),allGens,ns) ; 
    if ( !tmp )
      return ISMRC_AllocateError ;
    memset(tmp+os, 0, (ns-os)) ; 
    allGens = tmp ; 
    maxGen = genId ; 
  }
  return ISMRC_OK;
}
/*---------------------------------------------------------------------------*/

static int32_t internal_memRecoveryAddGeneration(ismStore_GenId_t genId, char *pData, uint64_t dataLength, uint8_t fActive)
{
  int rc ; 
  int gid = genId ; 
  ismStore_memGenInfo_t *gi ; 

  rc = ISMRC_OK;
  if ( (rc=extend_allGens(gid)) != ISMRC_OK ) return rc ; 
  gi = allGens + (gid-minGen) ;
  gi->genId = gid ; 
  if ( pData && dataLength > 0 )
  {
    ismStore_memGenHeader_t *pGenHeader = (ismStore_memGenHeader_t *)pData ; 
    gi->genSize = dataLength ;
    if ( pGenHeader->CompactSizeBytes )
      gi->state |= 32 ; // The caller have already called ism_store_memRecoveryLinkRefChanks
    else
    if ( fActive && !(gi->state&32) )
    {
      if ( (rc = ism_store_linkRefChanks(pGenHeader)) != ISMRC_OK )
        return rc ; 
      gi->state |= 32 ; 
    }
  }
  else
  if ( !gi->genSize )
  {
    pData = NULL ; 
    if ( (rc = ism_storeDisk_getGenerationInfo(gid, 0, NULL, &gi->genSize)) != StoreRC_OK )
    {
      if ( rc == StoreRC_Disk_IsNotOn ) rc = ISMRC_Error ; 
      if ( rc == StoreRC_SystemError  ) rc = ISMRC_Error ; 
      if ( rc == StoreRC_BadParameter ) rc = ISMRC_ArgNotValid ; 
      return rc ; 
    }
  }
  if ( gi->genSize > 0 )
  {
    if ( gi->genData )
    {
      if ( (gi->state&2) && !(gi->state&8) )
        ism_store_initGenMap(gi,1) ;
    }
    else
    {
      void *p = ism_store_getGenMem(gi->genSize, 0, gid, &rc) ; 
      if ( p )
      {
        if ( pData && dataLength > 0 )
        {
          gi->genData = p ; 
          gi->state  |= 3 ; 
          memcpy(p, pData, dataLength) ; 
          TRACE(5,"Generation %u is copied to memory ; gi->genSize %lu, curMem %lu\n",gid,gi->genSize,curMem);
        }
        else
        {
          ismStore_DiskBufferParams_t *bp ;
          ismStore_DiskTaskParams_t dtp[1] ; 
          gi->genData = p ; 
          gi->state   = 1 ; 

          memset(dtp,0,sizeof(dtp)) ; 
          dtp->fCancelOnTerm = 1 ; 
          dtp->Priority      = 1 ; 
          dtp->GenId         = gid ; 
          dtp->Callback      = ism_store_recCB ; 
          dtp->pContext      = gi ; 
          bp = dtp->BufferParams ; 
          bp->pBuffer      = gi->genData ; 
          bp->BufferLength = gi->genSize ; 
          if ( (rc = ism_storeDisk_readGeneration(dtp)) )
          {
            if ( rc == StoreRC_BadParameter ) rc = ISMRC_ArgNotValid ; 
            if ( rc == StoreRC_Disk_IsNotOn ) rc = ISMRC_Error ; 
            if ( rc == StoreRC_AllocateError) rc = ISMRC_AllocateError ;
          }
          TRACE(5,"Generation %u is read from disk ; gi->genSize %lu, curMem %lu\n",gid,gi->genSize,curMem);
        }
      }
    }
  }

  return rc ; 
}

/*---------------------------------------------------------------------------*/
static int internal_readAhead(void)
{
  ismStore_memMgmtHeader_t *pMgmHeader;
  ismStore_Handle_t handle ; 
  ismStore_memDescriptor_t *desc ;
  ismStore_memGenIdChunk_t *chunk ;
  ismStore_memGenHeader_t *inMemGens[3]={0,0,0};
  ismStore_memGenInfo_t *gi ; 
  int i, j, ni;

  pMgmHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress;
  if ( pMgmHeader->InMemGensCount > 2 )
  {
    TRACE(1," !!! Should not be here !!! pMgmHeader->InMemGensCount = %d\n",pMgmHeader->InMemGensCount);
    return -1 ; 
  }

  for ( i=0 ; i<pMgmHeader->InMemGensCount ; i++ )
    inMemGens[i] = (ismStore_memGenHeader_t *)(ismStore_memGlobal.pStoreBaseAddress + pMgmHeader->InMemGenOffset[i]) ;
  inMemGens[i++] = (ismStore_memGenHeader_t *)pMgmHeader ;
  ni = i <= 3 ? i : 3 ; 

  for ( handle=pMgmHeader->GenIdHandle ; handle ; handle=desc->NextHandle )
  {
    ismStore_GenId_t gid ; 
    desc = (ismStore_memDescriptor_t *)(ismStore_memGlobal.pStoreBaseAddress + ismSTORE_EXTRACT_OFFSET(handle)) ;
    chunk = (ismStore_memGenIdChunk_t *)((uintptr_t)desc+pMgmHeader->DescriptorStructSize) ;
    for ( i=0 ; i<chunk->GenIdCount ; i++ )
    {
      if ( isOn > 1 && params->MaxMemoryBytes - curMem >= params->MinMemoryBytes )
        break ; 
      gid = chunk->GenIds[i] ;
      if ( gid < minGen || gid > maxGen )
        continue ; 
      gi = allGens + (gid-minGen) ;
      if ( (gi->state & 256) )
        continue ; 
      for ( j=0 ; j<ni && gid != inMemGens[j]->GenId ; j++ ) ; /* empty body */
      if ( j >= ni && !(gi->state & 1) )
      {
        internal_memRecoveryAddGeneration(gid, NULL, 0, 0) ; 
        if ( !(gi->state & 1) )
          break ; 
      }
    }
  }
  return 0 ; 
}

/*---------------------------------------------------------------------------*/

int32_t ism_store_memRecoveryAddGeneration(ismStore_GenId_t genId, char *pData, uint64_t dataLength, uint8_t fActive)
{
  int rc ; 
  pthread_mutex_lock(&lock) ; 
  rc = internal_memRecoveryAddGeneration(genId, pData, dataLength, fActive) ; 
  pthread_mutex_unlock(&lock) ; 
  return rc ; 
}

/*---------------------------------------------------------------------------*/

static int32_t internal_memRecoveryDelGeneration(ismStore_GenId_t genId)
{
  int rc ; 
  int gid = genId ; 
  ismStore_memGenInfo_t *gi ; 

  rc = ISMRC_OK;
  if ( (rc=extend_allGens(gid)) != ISMRC_OK ) return rc ; 
  gi = allGens + (gid-minGen) ;
  TRACE(5, "memRecoveryDelGeneration: Gen deleted: gi->genId= %u, gi->genSize=%lu, gi->genData=%p, gi->state=%x\n", 
                                                   gi->genId,     gi->genSize,     gi->genData,    gi->state) ; 
  if ( gi->genSize && gi->genData && (gi->state&2) && !(gi->state&4) )
  {
    ism_common_free_memaligned(ism_memory_store_misc,gi->genData) ;
    curMem += gi->genSize ; 
    TRACE(5,"Recovery memory of size %lu has been freed from genId %u, curMem= %lu\n",gi->genSize,gi->genId,curMem);
  }
  if ( gi->genSizeMap )
  {
    ism_common_free_memaligned(ism_memory_store_misc,gi->genDataMap[0]) ;
    curMem += gi->genSizeMap ; 
    TRACE(5,"Recovery memory of size %lu has been freed from genId %u, curMem= %lu\n",gi->genSizeMap,gi->genId,curMem);
  }
  memset(gi,0,sizeof(ismStore_memGenInfo_t)) ; 
  internal_readAhead() ;
  return rc ; 
}

/*---------------------------------------------------------------------------*/

int32_t ism_store_memRecoveryDelGeneration(ismStore_GenId_t genId)
{
  int rc ; 
  pthread_mutex_lock(&lock) ; 
  rc = internal_memRecoveryDelGeneration(genId) ; 
  pthread_mutex_unlock(&lock) ; 
  return rc ; 
}

/*---------------------------------------------------------------------------*/

static int32_t internal_memRecoveryUpdGeneration(ismStore_GenId_t genId, uint64_t **pBitMaps, uint64_t predictedSizeBytes)
{
  int rc ; 
  ismStore_DiskBufferParams_t bp[1] ; 
  ismStore_memGenInfo_t *gi ; 

  rc = ISMRC_OK;
  memset(bp,0,sizeof(bp));
  bp->pBitMaps = pBitMaps ; 
  bp->fFreeMaps = 1 ; 
  gi = allGens + (genId-minGen) ;
  if ( (gi->state&7) == 3 )
  {
    int i ; 
    ismStore_memGenHeader_t *pGenHeader = (ismStore_memGenHeader_t *)gi->genData ;
    i = pGenHeader->CompactSizeBytes ? 0 : 1 ; 
    if ( (rc = ism_storeDisk_compactGenerationData(gi->genData, bp)) == ISMRC_OK )
    {
      ismStore_DiskTaskParams_t dtp[1] ; 
      curMem += gi->genSize - bp->BufferLength ;
      TRACE(5,"Recovery memory of size %lu has been freed from genId %u, curMem= %lu\n",gi->genSize - bp->BufferLength,gi->genId,curMem);
      if ( i )
        ism_common_free_memaligned(ism_memory_store_misc,gi->genData) ;
      gi->genData = bp->pBuffer ; 
      gi->genSize0= gi->genSize ; 
      gi->genSize = bp->BufferLength ; 

      memset(dtp,0,sizeof(dtp)) ; 
      dtp->fCancelOnTerm = 1 ; 
      dtp->Priority      = 1 ; 
      dtp->GenId         = genId ; 
      dtp->Callback      = ism_store_cmpCB ; 
      dtp->pContext      = gi ; 
      dtp->BufferParams->fAllocMem = 1 ; 

      rc = ism_storeDisk_writeGeneration(dtp) ; 
    }
    for ( i=0 ; i<ismSTORE_GRANULE_POOLS_COUNT ; i++ ) if ( pBitMaps[i]) ism_common_free(ism_memory_store_misc,pBitMaps[i]) ; 
    ism_common_free(ism_memory_store_misc,pBitMaps) ; 
  }
  else
  if ( (gi->state&7) == 1 )
  {
    if ( (gi->state&128) )
    {
      int i;
      for ( i=0 ; i<ismSTORE_GRANULE_POOLS_COUNT ; i++ ) if ( gi->pBitMaps[i]) ism_common_free(ism_memory_store_misc,gi->pBitMaps[i]) ; 
      ism_common_free(ism_memory_store_misc,gi->pBitMaps) ; 
    }
    gi->pBitMaps = pBitMaps ; 
    gi->state |= 128 ; 
  }
  else
  if ( !(gi->state&4) )
  {
    ismStore_DiskTaskParams_t dtp[1] ; 
    memset(dtp,0,sizeof(dtp)) ; 
    dtp->fCancelOnTerm = 1 ; 
    dtp->Priority      = 1 ; 
    dtp->GenId         = genId ; 
    dtp->Callback      = ism_store_cmpCB ;
    dtp->pContext      = gi ; 
    memcpy(dtp->BufferParams,bp,sizeof(bp)) ; 
    rc = ism_storeDisk_compactGeneration(dtp) ;
  }
  if ( rc )
  {
    if ( rc == StoreRC_Disk_TaskExists ) rc = ISMRC_OK ; 
    if ( rc == StoreRC_BadParameter    ) rc = ISMRC_ArgNotValid ; 
    if ( rc == StoreRC_Disk_IsNotOn    ) rc = ISMRC_Error ; 
    if ( rc == StoreRC_AllocateError   ) rc = ISMRC_AllocateError ;
  }
  return rc ; 
}

/*---------------------------------------------------------------------------*/

int32_t ism_store_memRecoveryUpdGeneration(ismStore_GenId_t genId, uint64_t **pBitMaps, uint64_t predictedSizeBytes)
{
  int rc ; 
  pthread_mutex_lock(&lock) ; 
  rc = internal_memRecoveryUpdGeneration(genId, pBitMaps, predictedSizeBytes) ; 
  pthread_mutex_unlock(&lock) ; 
  return rc ; 
}

/*---------------------------------------------------------------------------*/

int32_t ism_store_memRecoveryGetGeneration(ismStore_GenId_t genId, ismStore_DiskBufferParams_t *bp)
{
  int rc ; 
  ismStore_memGenInfo_t *gi ; 

  rc = ISMRC_OK;
  pthread_mutex_lock(&lock) ; 
  gi = allGens + (genId-minGen) ;
  if ( (gi->state&2) )
  {
    void *p ; 
    if ( posix_memalign(&p,ismStore_memGlobal.DiskBlockSizeBytes, gi->genSize) == 0 )
    {
      bp->pBuffer = p ; 
      bp->BufferLength = gi->genSize ; 
      memcpy(p, gi->genData, gi->genSize) ; 
    }
    else
      rc = ISMRC_AllocateError ; 
  } 
  else
    rc = ISMRC_StoreNotAvailable ; 
  pthread_mutex_unlock(&lock) ; 
  return rc ; 
}

/*---------------------------------------------------------------------------*/

int32_t ism_store_memRecoveryGetGenerationData(ismStore_GenId_t genId, ismStore_DiskBufferParams_t *bp)
{
  int rc ; 
  ismStore_memGenInfo_t *gi ; 

  rc = ISMRC_OK;
  pthread_mutex_lock(&lock) ; 
  gi = allGens + (genId-minGen) ;
  if ( isOn && genId >= minGen && genId <= maxGen && (gi->state&2) )
  {
    void *p = bp->pBuffer ; 
    size_t l = bp->BufferLength < gi->genSize ? bp->BufferLength : gi->genSize ; 
    if ( p && l > 0 )
    {
      memcpy(p, gi->genData, l) ; 
      bp->BufferLength = l ; 
    }
    else
      rc = ISMRC_AllocateError ; 
  } 
  else
    rc = ISMRC_StoreNotAvailable ; 
  pthread_mutex_unlock(&lock) ; 
  return rc ; 
}

/*---------------------------------------------------------------------------*/

static int32_t internal_memRecoveryStart(void)
{
  ismStore_memMgmtHeader_t *pMgmHeader;
  ismStore_Handle_t handle ; 
  ismStore_memDescriptor_t *desc ;
  ismStore_memGenIdChunk_t *chunk ;
  ismStore_memGenHeader_t *inMemGens[3]={0,0,0};
  uint64_t mgs=0 ; 
  register size_t DS;
  uint16_t genInd=0 ; 
  int i , ni , rc, mng, mxg;
  uint64_t offset, blocksize, upto ; 
#if USE_NEXT_OWNER
  uint64_t newOwnersSize[ISM_STORE_NUM_REC_TYPES];
  uint64_t prpOwnersSize[ISM_STORE_NUM_REC_TYPES];
#endif

  isOn = 2 ; 
  recTimes[1] = su_sysTime() ; 
  curGens = 0 ; 

  pMgmHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress;
  DS = pMgmHeader->DescriptorStructSize ; 
  for ( numGens=0 , mng=65535, mxg=0, handle=pMgmHeader->GenIdHandle ; handle ; handle=desc->NextHandle ) 
  {
    desc = (ismStore_memDescriptor_t *)(ismStore_memGlobal.pStoreBaseAddress + ismSTORE_EXTRACT_OFFSET(handle)) ;
    chunk = (ismStore_memGenIdChunk_t *)((uintptr_t)desc+DS) ;
    numGens += chunk->GenIdCount ;
    for ( i=0 ; i<chunk->GenIdCount ; i++ )
    {
      if ( mng > chunk->GenIds[i] )
           mng = chunk->GenIds[i] ;
      if ( mxg < chunk->GenIds[i] )
           mxg = chunk->GenIds[i] ;
    }
  }
  if ( (rc=extend_allGens(mng)) != ISMRC_OK ) return rc ; 
  if ( (rc=extend_allGens(mxg)) != ISMRC_OK ) return rc ; 

  if ( pMgmHeader->InMemGensCount > 2 )
  {
    TRACE(1," !!! Should not be here !!! pMgmHeader->InMemGensCount = %d\n",pMgmHeader->InMemGensCount);
    return -1 ; 
  }
  for ( i=0 ; i<pMgmHeader->InMemGensCount ; i++ )
    inMemGens[i] = (ismStore_memGenHeader_t *)(ismStore_memGlobal.pStoreBaseAddress + pMgmHeader->InMemGenOffset[i]) ;
  inMemGens[i++] = (ismStore_memGenHeader_t *)pMgmHeader ;
  ni = i <= 3 ? i : 3 ; 
  for ( i=0 ; i<ni ; i++ )
  {
    TRACE(8,"function %s, i %d, inMemGens[i] %p,  inMemGens[i]->GenId %hu, inMemGens[i]->MemSizeBytes %lu\n",
          __FUNCTION__, i,inMemGens[i],inMemGens[i]->GenId,inMemGens[i]->MemSizeBytes);
  }

  ism_storeDisk_removeCompactTasks() ; 

  gid1st = ismSTORE_MGMT_GEN_ID ; 
  rc = ISMRC_OK;
  for ( handle=pMgmHeader->GenIdHandle ; rc==ISMRC_OK && handle ; handle=desc->NextHandle )
  {
    ismStore_GenId_t gid ; 
    ismStore_memGenInfo_t *gi ; 
    desc = (ismStore_memDescriptor_t *)(ismStore_memGlobal.pStoreBaseAddress + ismSTORE_EXTRACT_OFFSET(handle)) ;
    chunk = (ismStore_memGenIdChunk_t *)((uintptr_t)desc+DS) ;
    for ( i=0 ; rc==ISMRC_OK && i<chunk->GenIdCount ; i++ )
    {
      int j;
      gid = chunk->GenIds[i] ;
      gi = allGens + (gid-minGen) ;
      gi->genId = gid ; 
      gi->genInd= genInd++ ; 
      if ( gid1st == ismSTORE_MGMT_GEN_ID && gid != ismSTORE_MGMT_GEN_ID )
        gid1st = gid ;
      for ( j=0 ; j<ni ; j++ )
      {
        if ( gid == inMemGens[j]->GenId )
        {
          if ( gi->genData )
          {
            TRACE(1,"recoveryStart: an allGens entry for inMem gen %u is already initialized: gi->genId= %u, gi->genSize=%lu, gi->genData=%p, gi->state=%x\n",gid,
                                                                                              gi->genId,     gi->genSize,     gi->genData,    gi->state) ; 
          }
          else
          {
            gi->genData = (char *)inMemGens[j] ; 
            gi->genSize = inMemGens[j]->MemSizeBytes ; 
            gi->state |= 7 ; 
            ism_store_initGenMap(gi,0) ;
          }
          break ; 
        }
      }
      if ( gi->state&4 )
        continue ; 
      if ( (rc = internal_memRecoveryAddGeneration(gid, NULL, 0, 0)) != ISMRC_OK )
        break ; 
      if ( mgs < gi->genSize )
           mgs = gi->genSize ;
      TRACE(9,"function %s, rc %d, maxMem %lu, curMem %lu, gi->genId %d, gi->genSize %lu ,gi->genData %p, gi->state %x\n",
            __FUNCTION__, rc, params->MaxMemoryBytes, curMem, gi->genId, gi->genSize, gi->genData, gi->state);
    }
  }
  if ( mgs > params->MaxMemoryBytes )
  {
    TRACE(1,"maxRecoveryMemory is not enough for the largest generations: %lu < %lu\n",params->MaxMemoryBytes,mgs);
    rc = ISMRC_ArgNotValid ; 
    ism_common_setErrorData(rc, "%s", ismSTORE_CFG_RECOVERY_MEMSIZE_MB);
  }

 #if USE_NEXT_OWNER
  ownersArraySize=0;
  memset(newOwnersSize,0,sizeof(newOwnersSize)) ;
  memset(prpOwnersSize,0,sizeof(prpOwnersSize)) ;
 #endif
  offset = upto = i = blocksize = 0 ; 
  for(;;)
  {
    if ( offset >= upto )
    {
      if ( pMgmHeader->PoolsCount > i && offset <= pMgmHeader->GranulePool[i].Offset )
      {
        blocksize = pMgmHeader->GranulePool[i].GranuleSizeBytes ; 
        offset    = pMgmHeader->GranulePool[i].Offset ; 
        upto      = offset + pMgmHeader->GranulePool[i].MaxMemSizeBytes ; 
        i++ ; 
      }
      else
        break ; 
    }
    desc = (ismStore_memDescriptor_t *)(ismStore_memGlobal.pStoreBaseAddress + offset) ;
    if ( ismSTORE_IS_SPLITITEM(desc->DataType) )
    {
      ismStore_memSplitItem_t *si = (ismStore_memSplitItem_t *)((uintptr_t)desc+DS) ;
      ismStore_GenId_t gid = ismSTORE_EXTRACT_OFFSET(si->hLargeData) ? ismSTORE_EXTRACT_GENID(si->hLargeData) : gid1st ;
      if ( gid >= minGen && gid <= maxGen && allGens[gid-minGen].genId == gid )
      {
        allGens[gid-minGen].isRecThere[T2T[desc->DataType]] = 1 ; 
       #if USE_NEXT_OWNER
        newOwnersSize[T2T[desc->DataType]]++ ; 
        ownersArraySize++;
       #endif
      }
      else
      {
        TRACE(5,"%s: an orphan owner found at offset 0x%lx\n",__FUNCTION__,offset);
      }
     #if USE_NEXT_OWNER
      if ( hasProp[T2T[desc->DataType]] ) 
      {
        gid = ismSTORE_EXTRACT_GENID(desc->Attribute) ; 
        if ( gid >= minGen && gid <= maxGen && allGens[gid-minGen].genId == gid )
        {
          prpOwnersSize[T2T[desc->DataType]]++ ; 
          ownersArraySize++ ; 
        }
      }
     #endif
    }
    offset += blocksize ; 
  }
 #if USE_NEXT_OWNER
  recTimes[6] = su_sysTime() ; 
  if ( ownersArraySize )
  {
    size_t off;
    if (!(ownersArray = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,56),ownersArraySize*sizeof(ismStore_Handle_t))) )
    {
      TRACE(1,"malloc failed for ownersArray (%lu bytes)\n",ownersArraySize*sizeof(ismStore_Handle_t)) ; 
      rc = ISMRC_AllocateError ;
      goto exit ; 
    }
    TRACE(5," %lu bytes allocated to speedup array ownersArray.\n",ownersArraySize*sizeof(ismStore_Handle_t));
    for ( i=0, off=0 ; i<ISM_STORE_NUM_REC_TYPES ; i++ )
    {
      if ( newOwnersSize[i] )
      {
        newOwners[i].OwnersSize = newOwnersSize[i] ; 
        newOwners[i].Owners = ownersArray + off ; 
        off += newOwnersSize[i] ; 
      }
      if ( prpOwnersSize[i] )
      {
        prpOwners[i].OwnersSize = prpOwnersSize[i] ; 
        prpOwners[i].Owners = ownersArray + off ; 
        off += prpOwnersSize[i] ; 
      }
    }
    memset(newOwnersSize,0,sizeof(newOwnersSize)) ;
    memset(prpOwnersSize,0,sizeof(prpOwnersSize)) ;
    offset = upto = i = blocksize = 0 ; 
    for(;;)
    {
      if ( offset >= upto )
      {
        if ( pMgmHeader->PoolsCount > i && offset <= pMgmHeader->GranulePool[i].Offset )
        {
          blocksize = pMgmHeader->GranulePool[i].GranuleSizeBytes ; 
          offset    = pMgmHeader->GranulePool[i].Offset ; 
          upto      = offset + pMgmHeader->GranulePool[i].MaxMemSizeBytes ; 
          i++ ; 
        }
        else
          break ; 
      }
      desc = (ismStore_memDescriptor_t *)(ismStore_memGlobal.pStoreBaseAddress + offset) ;
      if ( ismSTORE_IS_SPLITITEM(desc->DataType) )
      {
        int j = T2T[desc->DataType] ; 
        ismStore_memSplitItem_t *si = (ismStore_memSplitItem_t *)((uintptr_t)desc+DS) ;
        ismStore_GenId_t gid = ismSTORE_EXTRACT_OFFSET(si->hLargeData) ? ismSTORE_EXTRACT_GENID(si->hLargeData) : gid1st ;
        if ( gid >= minGen && gid <= maxGen && allGens[gid-minGen].genId == gid )
        {
          gid = allGens[gid-minGen].genInd ; 
          newOwners[j].Owners[newOwnersSize[j]++] = ismSTORE_BUILD_HANDLE(gid,offset) ; 
        }
        if ( hasProp[T2T[desc->DataType]] ) 
        {
          gid = ismSTORE_EXTRACT_GENID(desc->Attribute) ; 
          if ( gid >= minGen && gid <= maxGen && allGens[gid-minGen].genId == gid )
          {
            gid = allGens[gid-minGen].genInd ; 
            prpOwners[j].Owners[prpOwnersSize[j]++] = ismSTORE_BUILD_HANDLE(gid,offset) ; 
          }
        }
    }
    offset += blocksize ; 
  }
  }
  for ( i=0 ; i<ISM_STORE_NUM_REC_TYPES ; i++ )
  {
    if ( newOwners[i].OwnersSize )
      qsort(newOwners[i].Owners, newOwners[i].OwnersSize, sizeof(uint64_t), qcmp3);
    if ( prpOwners[i].OwnersSize )
      qsort(prpOwners[i].Owners, prpOwners[i].OwnersSize, sizeof(uint64_t), qcmp3);
  }
 exit:
 #endif

  {
    ismStore_DiskTaskParams_t dtp[1] ; 
    memset(dtp,0,sizeof(dtp)) ; 
    dtp->fCancelOnTerm = 1 ; 
    dtp->Priority      = 0 ; 
    dtp->Callback      = ism_store_cmpDO ; 
    ism_storeDisk_deleteDeadFiles(dtp) ; 
  }
    
  recTimes[2] = su_sysTime() ; 
  return rc ;
}
/*---------------------------------------------------------------------------*/
int32_t ism_store_memRecoveryStart(void)
{
  int rc;

  pthread_mutex_lock(&lock) ; 
  rc = internal_memRecoveryStart() ; 
  pthread_mutex_unlock(&lock) ; 
  return rc ; 
}
/*---------------------------------------------------------------------------*/

XAPI int32_t ism_store_memRecoveryTerm(void)
{
  int i, rc ; 
  ismStore_memGenInfo_t *gi ; 

#if USE_REC_TIME
 TRACE(1,"RecoveryTiming: recTime = %f\n",recTime);
#endif

  pthread_mutex_lock(&lock) ; 
  rc = ISMRC_OK ;
  if ( isOn )
  {
	for (i = maxGen - minGen; i >= 0; i--) {
		gi = allGens + i;
#if USE_NEXT_OWNER
		if (gi->ownersArray && gi->ownersArraySize) {
			ism_common_free_memaligned(ism_memory_store_misc,
					gi->ownersArray);
			gi->ownersArray = NULL;
			gi->ownersArraySize = 0;
		}
#endif
		if (gi->state & 4)
			continue;
		if (gi->genSizeMap) {
			ism_common_free_memaligned(ism_memory_store_misc, gi->genDataMap[0]);
			gi->genDataMap[0] = NULL;
			gi->genSizeMap = 0;
		}
		if (!gi->genData)
			continue;
		while ((gi->state & 3) == 1) /* BEAM suppression: infinite loop */
			pthread_cond_wait(&cond, &lock);
		ism_common_free_memaligned(ism_memory_store_misc, gi->genData);
		gi->genData = NULL;
		gi->state = 0;
	}
    ism_common_free(ism_memory_store_misc,allGens) ; 
    allGens = NULL ;
    minGen= maxGen= isOn=0 ;
    memset(prevGens,0,sizeof(prevGens));
#if USE_NEXT_OWNER
	if (ownersArraySize) {
		ism_common_free(ism_memory_store_misc, ownersArray);
		ownersArray = NULL;
		ownersArraySize = 0;
	}
	memset(newOwners, 0, sizeof(newOwners));
	memset(prpOwners, 0, sizeof(prpOwners));
#endif
    {
      double *d = recTimes ; 
      d[5] = su_sysTime() ; 
      d[0] = viewTime ; 
      if ( d[5] >= d[4] &&
           d[4] >= d[3] &&
           d[3] >= d[2] &&
           d[2] >= d[1] &&
           d[1] >= d[0] &&
           d[0] > 0e0 )
      {
        TRACE(5,"Recovery Summary Timing: viewChange-to-recoveryStart=%f , recoveryStart=%f(%f) , f1st genId=%f, last genId=%f, recoveryTerm=%f, wait4Disk=%f, buildRefOwners=%f, numGens=%d\n",
              d[1]-d[0],d[2]-d[1],d[2]-d[6],d[3]-d[2],d[4]-d[3],d[5]-d[4],d[7],d[8],numGens) ;
      }
      memset(recTimes,0,sizeof(recTimes));
    }
  } 
  pthread_mutex_unlock(&lock) ;
  return rc;
}

/*---------------------------------------------------------------------------*/

#if USE_REC_TIME
static int32_t ism_store_memGetNextGenId_(ismStore_IteratorHandle *pIterator, ismStore_GenId_t *pGenId);
XAPI int32_t ism_store_memGetNextGenId(ismStore_IteratorHandle *pIterator, ismStore_GenId_t *pGenId)
{
  double t;
  int32_t rc ; 
  t = ism_common_readTSC() ; 
  rc = ism_store_memGetNextGenId_(pIterator, pGenId);
  recTime += ism_common_readTSC()-t ; 
  return rc ; 
}
static int32_t ism_store_memGetNextGenId_(ismStore_IteratorHandle *pIterator, ismStore_GenId_t *pGenId)
#else
XAPI int32_t ism_store_memGetNextGenId(ismStore_IteratorHandle *pIterator, ismStore_GenId_t *pGenId)
#endif
{
  ismStore_memMgmtHeader_t *pMgmHeader;
  ismStore_IteratorHandle  pIter ; 
  ismStore_memDescriptor_t *desc ;
  ismStore_memGenIdChunk_t *chunk ;

  if ( !pIterator )
    return ISMRC_NullArgument ;

  pMgmHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress;
  pIter = *pIterator ; 
  if ( !pIter )
  {
    pIter = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,62),sizeof(ismStore_Iterator_t)) ; 
    if ( !pIter )
      return ISMRC_AllocateError ;
    memset(pIter,0,sizeof(ismStore_Iterator_t)) ; 
    pIter->type = ISM_STORE_ITERTYPE_GENID ; 
    pIter->handle= pMgmHeader->GenIdHandle ; 
    pIter->index = 0 ; 
    *pIterator = pIter ; 
    recTimes[3] = su_sysTime() ; 
  }
  if ( pIter->type != ISM_STORE_ITERTYPE_GENID )
  {
    ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "pIterator");
    return ISMRC_ArgNotValid ;
  }

  for(;;)
  {
    desc = (ismStore_memDescriptor_t *)(ismStore_memGlobal.pStoreBaseAddress + ismSTORE_EXTRACT_OFFSET(pIter->handle)) ;
    chunk = (ismStore_memGenIdChunk_t *)((uintptr_t)desc+pMgmHeader->DescriptorStructSize) ;
    if ( pIter->index >= chunk->GenIdCount )
    {
      if (!desc->NextHandle )
      {
        ism_common_free(ism_memory_store_misc,pIter); 
        *pIterator = NULL ; 
        recTimes[4] = su_sysTime() ; 
        return ISMRC_StoreNoMoreEntries ;
      }
      pIter->handle= desc->NextHandle ; 
      pIter->index = 0 ; 
    }
    else
    {
      *pGenId = chunk->GenIds[pIter->index] ;   
      pIter->index++ ;
      prevGens[0] = prevGens[1] ; 
      prevGens[1] = prevGens[2] ; 
      prevGens[2] = *pGenId ; 
      if ( prevGens[0] >= minGen && prevGens[0] <= maxGen ) 
      {
        ismStore_memGenInfo_t *gi ; 
        gi = allGens + (prevGens[0]-minGen) ;
        if ( (gi->state&7) == 3 && gi->genSize && gi->genData )
        {
          size_t pcm=curMem ; 
          pthread_mutex_lock(&lock) ; 
          ism_common_free_memaligned(ism_memory_store_misc,gi->genData) ;
          gi->genData = NULL ; 
          gi->state &= ~(1|2|64) ; 
          curMem += gi->genSize ; 
          if ( gi->genSizeMap > 0 )
          {
            ism_common_free_memaligned(ism_memory_store_misc,gi->genDataMap[0]) ;
            curMem += gi->genSizeMap ; 
            gi->genDataMap[0] = NULL ; 
            gi->genSizeMap    = 0 ; 
          }
          TRACE(5,"Recovery memory of size %lu has been freed from genId %u, curMem= %lu\n",curMem-pcm,gi->genId,curMem);
          internal_readAhead() ;
          pthread_mutex_unlock(&lock) ; 
        }
      }
      if ( prevGens[1] >= minGen && prevGens[1] <= maxGen ) 
      {
        ismStore_memGenInfo_t *gi ; 
        gi = allGens + (prevGens[1]-minGen) ;
        gi->useTime = su_sysTime() ; 
        gi->state |= 256 ; 
      }
      curGens++ ; 
      return ISMRC_OK ;
    }
  }
}

/*---------------------------------------------------------------------------*/

static int copyRecordData(char *genData, ismStore_GenId_t gid, ismStore_Handle_t offset, ismStore_Record_t *pRecord)
{
  int i, rc, setLen=0 ;
  register size_t DS ; 
  size_t off0, off1 , l0, l1, l, setOff=offset ; 
  ismStore_memGenHeader_t  *pGenHeader;
  ismStore_memDescriptor_t *desc ;

  pGenHeader = (ismStore_memGenHeader_t *)genData ;
  DS = pGenHeader->DescriptorStructSize ; 
  if ( pGenHeader->CompactSizeBytes )
  {
    char *bptr,*sptr ; 
    ismStore_memGenInfo_t *gi ; 
    if ( (rc = ism_store_getGenMap(gid)) != ISMRC_OK )
      goto done ; 
    gi = allGens + (gid-minGen) ;
    if (!(desc = off2desc(gi, offset)) )
    {
      rc = ISMRC_Error ;
      goto done ; 
    }
    sptr = bptr = (char *)desc ; 
    for(off0=0,i=0,off1=0;;)
    {
      l0 = desc->DataLength - off0 ; 
      l1 = pRecord->pFragsLengths[i] - off1 ; 
      l = l0 < l1 ? l0 : l1 ; 
      memcpy(pRecord->pFrags[i]+off1,(void *)((uintptr_t)desc+DS+off0),l) ; 
      off0 += l ; 
      off1 += l ; 
      if ( off0 >= desc->DataLength )
      {
        bptr += ALIGNED(DS+desc->DataLength) ;
        if( !ismSTORE_EXTRACT_OFFSET(desc->NextHandle) )
          break ; 
        desc = (ismStore_memDescriptor_t *)bptr ; 
        off0 = 0 ; 
      }
      if ( off1 >= pRecord->pFragsLengths[i] )
      {
        i++ ; 
        off1 = 0 ; 
      }
    }
    setLen = bptr - sptr ; 
  }
  else
  {
    desc = (ismStore_memDescriptor_t *)(genData + offset) ;
    setLen += DS + desc->DataLength ; 
    for(off0=0,i=0,off1=0;;)
    {
      l0 = desc->DataLength - off0 ; 
      l1 = pRecord->pFragsLengths[i] - off1 ; 
      l = l0 < l1 ? l0 : l1 ; 
      memcpy(pRecord->pFrags[i]+off1,(void *)((uintptr_t)desc+DS+off0),l) ; 
      off0 += l ; 
      off1 += l ; 
      if ( off0 >= desc->DataLength )
      {
        offset = ismSTORE_EXTRACT_OFFSET(desc->NextHandle) ; 
        if ( !offset )
          break ; 
        desc = (ismStore_memDescriptor_t *)(genData + offset) ;
        setLen += DS + desc->DataLength ; 
        off0 = 0 ; 
      }
      if ( off1 >= pRecord->pFragsLengths[i] )
      {
        i++ ; 
        off1 = 0 ; 
      }
    }
  }
  rc = ISMRC_OK ;
 done:
  ism_store_memSetGenMap(ismStore_memGlobal.pGensMap[gid], ismSTORE_BITMAP_LIVE_IDX, setOff, setLen);
  return rc ;
}
/*---------------------------------------------------------------------------*/

#if USE_REC_TIME
static int32_t ism_store_memGetNextRecordForType_(ismStore_IteratorHandle *pIterator, ismStore_RecordType_t recordType, ismStore_GenId_t genId, ismStore_Handle_t *pHandle, ismStore_Record_t *pRecord);
XAPI int32_t ism_store_memGetNextRecordForType(ismStore_IteratorHandle *pIterator, ismStore_RecordType_t recordType, ismStore_GenId_t genId, ismStore_Handle_t *pHandle, ismStore_Record_t *pRecord)
{
  double t;
  int32_t rc ; 
  t = ism_common_readTSC() ; 
  rc = ism_store_memGetNextRecordForType_(pIterator, recordType, genId, pHandle, pRecord);
  recTime += ism_common_readTSC()-t ; 
  return rc ; 
}
static int32_t ism_store_memGetNextRecordForType_(ismStore_IteratorHandle *pIterator, ismStore_RecordType_t recordType, ismStore_GenId_t genId, ismStore_Handle_t *pHandle, ismStore_Record_t *pRecord)
#else
XAPI int32_t ism_store_memGetNextRecordForType(ismStore_IteratorHandle *pIterator,
                                            ismStore_RecordType_t recordType,
                                            ismStore_GenId_t genId,
                                            ismStore_Handle_t *pHandle,
                                            ismStore_Record_t *pRecord)
#endif
{
  register size_t DS ; 
  ismStore_memGenHeader_t  *pGenHeader;
  ismStore_IteratorHandle  pIter ; 
  ismStore_GenId_t gid = genId , gid0 ; 
  char *genData ;
  int rc ; 

  if ( !(pIterator && pHandle && pRecord) )
  {
    TRACE(1,"Bad arguments: function %s, pIterator %p , pHandle %p , pRecord %p\n", __FUNCTION__, pIterator, pHandle, pRecord) ;
    return ISMRC_NullArgument ;
  }

  pIter = *pIterator ; 
  if ( !pIter )
  {
    int isSi=0;
    if ( recordType == ISM_STORE_RECTYPE_SERVER )
    {
      if ( gid != ismSTORE_MGMT_GEN_ID )
      {
      //printf("@@@ 0 records of type %s for gen %u returned by %s\n",recName(recordType),genId,__func__);
        return ISMRC_StoreNoMoreEntries ;
      }
      genData = ismStore_memGlobal.pStoreBaseAddress;
      gid0    = ismSTORE_MGMT_GEN_ID ; 
    }
    else
    if ( recordType >= ISM_STORE_RECTYPE_MSG && recordType <= ISM_STORE_RECTYPE_RPROP )
    {
      if (!(genData = ism_store_getGen(gid,&rc)) )
        return rc ; 
      gid0 = genId ; 
      if ( !allGens[gid-minGen].isRecThere[T2T[recordType]] )
      {
      //printf("@@@ 0 records of type %s for gen %u returned by %s\n",recName(recordType),genId,__func__);
        return ISMRC_StoreNoMoreEntries ;
    }
    }
    else
    if ( ismSTORE_IS_SPLITITEM(recordType) )
    {
      gid0    = ismSTORE_MGMT_GEN_ID ; 
      if ( !allGens[gid-minGen].isRecThere[T2T[recordType]] )
      {
      //printf("@@@ 0 records of type %s for gen %u returned by %s\n",recName(recordType),genId,__func__);
        return ISMRC_StoreNoMoreEntries ;
      }
      genData = ismStore_memGlobal.pStoreBaseAddress;
      isSi++ ; 
    }
    else
    {
      ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "recordType");
      return ISMRC_ArgNotValid ;
    }

    pIter = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,66),sizeof(ismStore_Iterator_t)) ; 
    if ( !pIter )
      return ISMRC_AllocateError ;
    memset(pIter,0,sizeof(ismStore_Iterator_t)) ; 
    pIter->type = ISM_STORE_ITERTYPE_RECORD; 
    pIter->genId = genId ; 
    pIter->recType = recordType ; 
    pIter->genData = genData ; 
    pIter->isSi    = isSi ; 
    pIter->gid0    = gid0 ; 
    pGenHeader = (ismStore_memGenHeader_t *)genData ;
    if ( pGenHeader->CompactSizeBytes )
    {
      pIter->bptr = (char *)pGenHeader + pGenHeader->GranulePool[0].Offset ; 
      pIter->eptr = (char *)pGenHeader + pGenHeader->CompactSizeBytes ; 
    }
    *pIterator = pIter ; 
  }

  if ( pIter && (pIter->genId != genId || pIter->recType != recordType || pIter->type != ISM_STORE_ITERTYPE_RECORD) )
  {
    TRACE(1,"Bad arguments: function %s, pIter %p, pIter->genId %hu, genId %hu, pIter->recType %d, recordType %d\n",
          __FUNCTION__, pIter, pIter->genId, genId, pIter->recType, recordType);
    ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "pIterator");
    return ISMRC_ArgNotValid ;
  }

  genData = pIter->genData ; 
  gid0    = pIter->gid0 ; 

  pGenHeader = (ismStore_memGenHeader_t *)genData ;
  DS = pGenHeader->DescriptorStructSize ; 

  if ( pGenHeader->CompactSizeBytes )
  {
    ismStore_memDescriptor_t *desc ;
    ismStore_memGranulePool_t *pPool;
  
    while ( pIter->bptr < pIter->eptr )
    {
      desc = (ismStore_memDescriptor_t *)pIter->bptr ; 
      pPool = &pGenHeader->GranulePool[desc->PoolId] ; 
      pIter->offset = pPool->Offset + (size_t)desc->GranuleIndex * pPool->GranuleSizeBytes ; 

      if ( (ismStore_RecordType_t)desc->DataType != recordType )
      {
        pIter->bptr += ALIGNED(DS+desc->DataLength) ; 
        continue ; 
      }
  
      if ( pIter->isSi )
      {
        ismStore_Handle_t offset ; 
        uint64_t l0 = pRecord->DataLength ; 
        ismStore_memSplitItem_t *si = (ismStore_memSplitItem_t *)((uintptr_t)desc+DS) ;
        uint64_t l1 = si->DataLength   ; 
        gid = ismSTORE_EXTRACT_OFFSET(si->hLargeData) ? ismSTORE_EXTRACT_GENID(si->hLargeData) : gid1st ;
        if ( gid != genId )
        {
          pIter->bptr += ALIGNED(DS+desc->DataLength) ; 
          continue ; 
        }
        *pHandle = ismSTORE_BUILD_HANDLE(gid0,pIter->offset) ; 
        pRecord->Type      = (ismStore_RecordType_t)desc->DataType ; 
        pRecord->Attribute = desc->Attribute ; 
        pRecord->State     = desc->State ; 
        pRecord->DataLength = l1 ;
        if ( l0 < l1 )
          return ISMRC_StoreBufferTooSmall ;
        if (!(offset = ismSTORE_EXTRACT_OFFSET(si->hLargeData)) )
        {
          int j;
          size_t l;
          for ( j=0, l0=0 ; l0<pRecord->DataLength ;)
          {
            l = pRecord->pFragsLengths[j] < pRecord->DataLength - l0 ? pRecord->pFragsLengths[j] : pRecord->DataLength - l0 ; 
            memcpy(pRecord->pFrags[j],(void *)((uintptr_t)si+pGenHeader->SplitItemStructSize+l0),l) ; 
            l0 += l ; 
            j++ ; 
          }
          pIter->bptr += ALIGNED(DS+desc->DataLength) ; 
        //pIter->refCount++;
          return ISMRC_OK ;
        }
  
        if (!(genData = ism_store_getGen(gid,&rc)) )
          return rc ; 
        copyRecordData(genData, gid, offset, pRecord) ; 
        pIter->bptr += ALIGNED(DS+desc->DataLength) ; 
      //pIter->refCount++;
        return ISMRC_OK ;
      }
      else
      {
        uint64_t l0 = pRecord->DataLength ; 
        uint64_t l1 = desc->TotalLength ;
        pRecord->DataLength = l1 ;
        *pHandle = ismSTORE_BUILD_HANDLE(gid0,pIter->offset) ; 
        pRecord->Type      = (ismStore_RecordType_t)desc->DataType ; 
        pRecord->Attribute = desc->Attribute ; 
        pRecord->State     = desc->State ; 
        if ( l0 < l1 )
          return ISMRC_StoreBufferTooSmall ;
        copyRecordData(genData, gid0, pIter->offset, pRecord) ; 
        pIter->bptr += ALIGNED(DS+desc->DataLength) ; 
      //pIter->refCount++;
        return ISMRC_OK ;
      }
    }
  //printf("@@@ %u records of type %s for gen %u returned by %s\n",pIter->refCount,recName(recordType),genId,__func__);
    ism_common_free(ism_memory_store_misc,pIter); 
    *pIterator = NULL ; 
    return ISMRC_StoreNoMoreEntries ;
  }
  else
  {
    ismStore_memDescriptor_t *desc ;
    for(;;)
    {
      if ( pIter->offset >= pIter->upto )
      {
        int i = pIter->poolId ; 
        if ( pGenHeader->PoolsCount > i && pIter->offset <= pGenHeader->GranulePool[i].Offset )
        {
          pIter->blockSize = pGenHeader->GranulePool[i].GranuleSizeBytes ; 
          pIter->offset    = pGenHeader->GranulePool[i].Offset ; 
          pIter->upto      = pIter->offset + pGenHeader->GranulePool[i].MaxMemSizeBytes ; 
          pIter->poolId++ ; 
        }
        else
        {
        //printf("@@@ %u records of type %s for gen %u returned by %s\n",pIter->refCount,recName(recordType),genId,__func__);
          ism_common_free(ism_memory_store_misc,pIter); 
          *pIterator = NULL ; 
          return ISMRC_StoreNoMoreEntries ;
        }
      }
      desc = (ismStore_memDescriptor_t *)(genData + pIter->offset) ;
      if ( (ismStore_RecordType_t)desc->DataType != recordType )
      {
        pIter->offset += pIter->blockSize ; 
        continue ; 
      }
  
      if ( pIter->isSi )
      {
        ismStore_Handle_t offset ; 
        uint64_t l0 = pRecord->DataLength ; 
        ismStore_memSplitItem_t *si = (ismStore_memSplitItem_t *)((uintptr_t)desc+DS) ;
        uint64_t l1 = si->DataLength   ; 
        gid = ismSTORE_EXTRACT_OFFSET(si->hLargeData) ? ismSTORE_EXTRACT_GENID(si->hLargeData) : gid1st ;
        if ( gid != genId )
        {
          pIter->offset += pIter->blockSize ; 
          continue ; 
        }
        *pHandle = ismSTORE_BUILD_HANDLE(gid0,pIter->offset) ; 
        pRecord->Type      = (ismStore_RecordType_t)desc->DataType ; 
        pRecord->Attribute = desc->Attribute ; 
        pRecord->State     = desc->State ; 
        pRecord->DataLength = l1 ;
        if ( l0 < l1 )
          return ISMRC_StoreBufferTooSmall ;
        if (!(offset = ismSTORE_EXTRACT_OFFSET(si->hLargeData)) )
        {
          int j;
          size_t l;
          for ( j=0, l0=0 ; l0<pRecord->DataLength ;)
          {
            l = pRecord->pFragsLengths[j] < pRecord->DataLength - l0 ? pRecord->pFragsLengths[j] : pRecord->DataLength - l0 ; 
            memcpy(pRecord->pFrags[j],(void *)((uintptr_t)si+pGenHeader->SplitItemStructSize+l0),l) ; 
            l0 += l ; 
            j++ ; 
          }
          pIter->offset += pIter->blockSize ; 
        //pIter->refCount++;
          return ISMRC_OK ;
        }
  
        if (!(genData = ism_store_getGen(gid,&rc)) )
          return rc ; 
        pGenHeader = (ismStore_memGenHeader_t *)genData ;
        copyRecordData(genData, gid, offset, pRecord) ; 
        pIter->offset += pIter->blockSize ; 
      //pIter->refCount++;
        return ISMRC_OK ;
      }
      else
      {
        uint64_t l0 = pRecord->DataLength ; 
        uint64_t l1 = desc->TotalLength ;
        pRecord->DataLength = l1 ;
        *pHandle = ismSTORE_BUILD_HANDLE(gid0,pIter->offset) ; 
        pRecord->Type      = (ismStore_RecordType_t)desc->DataType ; 
        pRecord->Attribute = desc->Attribute ; 
        pRecord->State     = desc->State ; 
        if ( l0 < l1 )
          return ISMRC_StoreBufferTooSmall ;
        copyRecordData(genData, gid0, pIter->offset, pRecord) ; 
        pIter->offset += pIter->blockSize ; 
      //pIter->refCount++;
        return ISMRC_OK ;
      }
    }
  }
}


/*---------------------------------------------------------------------------*/
#if USE_NEXT_OWNER
#if USE_REC_TIME
static int32_t ism_store_getNextNewOwner_(ismStore_IteratorHandle *pIterator, ismStore_RecordType_t recordType, ismStore_GenId_t genId, ismStore_Handle_t *pHandle, ismStore_Record_t *pRecord);
XAPI   int32_t ism_store_getNextNewOwner (ismStore_IteratorHandle *pIterator, ismStore_RecordType_t recordType, ismStore_GenId_t genId, ismStore_Handle_t *pHandle, ismStore_Record_t *pRecord)
{
  double t;
  int32_t rc ; 
  t = ism_common_readTSC() ; 
  rc = ism_store_getNextNewOwner_(pIterator, recordType, genId, pHandle, pRecord);
  recTime += ism_common_readTSC()-t ; 
  return rc ; 
}
static int32_t ism_store_getNextNewOwner_(ismStore_IteratorHandle *pIterator, ismStore_RecordType_t recordType, ismStore_GenId_t genId, ismStore_Handle_t *pHandle, ismStore_Record_t *pRecord)
#else
XAPI   int32_t ism_store_getNextNewOwner (ismStore_IteratorHandle *pIterator, ismStore_RecordType_t recordType, ismStore_GenId_t genId, ismStore_Handle_t *pHandle, ismStore_Record_t *pRecord)
#endif
{
  register size_t DS ; 
  ismStore_memGenHeader_t  *pGenHeader;
  ismStore_IteratorHandle  pIter ; 
  ismStore_GenId_t gid = genId , gind ; 
  ismStore_Handle_t handle ;
  char *genData ;
  int rc ; 

  if ( !(pIterator && pHandle && pRecord) )
  {
    TRACE(1,"Bad arguments: function %s, pIterator %p , pHandle %p , pRecord %p\n", __FUNCTION__, pIterator, pHandle, pRecord) ;
    return ISMRC_NullArgument ;
  }

  pIter = *pIterator ; 
  if ( !pIter )
  {
    int j = T2T[recordType] ;
    ismStore_ownerByInd_t *Owners = &newOwners[j]; 
    if (!Owners->Owners || !Owners->OwnersSize || Owners->OwnersInd >= Owners->OwnersSize )
    {
      return ISMRC_StoreNoMoreEntries ;
    }
    gind = allGens[gid-minGen].genInd ; 
    while ( Owners->OwnersInd < Owners->OwnersSize && ismSTORE_EXTRACT_GENID(Owners->Owners[Owners->OwnersInd]) < gind ) Owners->OwnersInd++ ; 
    if ( Owners->OwnersInd >= Owners->OwnersSize || ismSTORE_EXTRACT_GENID(Owners->Owners[Owners->OwnersInd]) > gind )
    {
      return ISMRC_StoreNoMoreEntries ;
    }

    if (!(genData = ism_store_getGen(ismSTORE_MGMT_GEN_ID,&rc)) )
      return rc ; 
    pIter = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,69),sizeof(ismStore_Iterator_t)) ; 
    if ( !pIter )
      return ISMRC_AllocateError ;
    memset(pIter,0,sizeof(ismStore_Iterator_t)) ; 
    pIter->type = ISM_STORE_ITERTYPE_RECORD; 
    pIter->genId = genId ; 
    pIter->genData = genData ; 
    pIter->gid0    = gind ; 
    pIter->Owners  = Owners ; 
    pIter->index   = Owners->OwnersInd ; 
    *pIterator = pIter ; 
  }

  if ( pIter && (pIter->genId != genId || pIter->type != ISM_STORE_ITERTYPE_RECORD) )
  {
    TRACE(1,"Bad arguments: function %s, pIter %p, pIter->genId %hu, genId %hu\n",
          __FUNCTION__, pIter, pIter->genId, genId);
    ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "pIterator");
    return ISMRC_ArgNotValid ;
  }

  if ( pIter->index < pIter->Owners->OwnersSize )
  {
    handle = pIter->Owners->Owners[pIter->index] ; 
    gind =  ismSTORE_EXTRACT_GENID(handle) ; 
    if ( gind == pIter->gid0 )
    {
      uint64_t l0, l1;
      ismStore_memDescriptor_t *desc ;
      ismStore_memSplitItem_t *si ; 
      ismStore_Handle_t offset ; 
  
      genData = pIter->genData ; 
      pGenHeader = (ismStore_memGenHeader_t *)genData ;
      DS = pGenHeader->DescriptorStructSize ; 
      offset = ismSTORE_EXTRACT_OFFSET(handle) ; 
      desc = (ismStore_memDescriptor_t *)(genData + offset) ;
      l0 = pRecord->DataLength ; 
      si = (ismStore_memSplitItem_t *)((uintptr_t)desc+DS) ;
      l1 = si->DataLength   ; 
      *pHandle = ismSTORE_BUILD_HANDLE(ismSTORE_MGMT_GEN_ID,offset) ; 
      pRecord->Type      = (ismStore_RecordType_t)desc->DataType ; 
      pRecord->Attribute = desc->Attribute ; 
      pRecord->State     = desc->State ; 
      pRecord->DataLength = l1 ;
      if ( l0 < l1 )
        return ISMRC_StoreBufferTooSmall ;
      if (!(offset = ismSTORE_EXTRACT_OFFSET(si->hLargeData)) )
      {
        int j;
        size_t l;
        for ( j=0, l0=0 ; l0<pRecord->DataLength ;)
      {
          l = pRecord->pFragsLengths[j] < pRecord->DataLength - l0 ? pRecord->pFragsLengths[j] : pRecord->DataLength - l0 ; 
          memcpy(pRecord->pFrags[j],(void *)((uintptr_t)si+pGenHeader->SplitItemStructSize+l0),l) ; 
          l0 += l ; 
          j++ ; 
        }
      //pIter->refCount++ ; 
        pIter->index++ ; 
        return ISMRC_OK ;
      }
  
      if (!(genData = ism_store_getGen(gid,&rc)) )
        return rc ; 
      copyRecordData(genData, gid, offset, pRecord) ; 
    //pIter->refCount++ ; 
      pIter->index++ ;
      return ISMRC_OK ;
    }
  }

//printf("@@@ %u records for gen %u returned by %s\n",pIter->refCount,genId,__func__);
        ism_common_free(ism_memory_store_misc,pIter); 
        *pIterator = NULL ; 
        return ISMRC_StoreNoMoreEntries ;
      }

/*---------------------------------------------------------------------------*/

#if USE_REC_TIME
static int32_t ism_store_getNextPropOwner_(ismStore_IteratorHandle *pIterator, ismStore_RecordType_t recordType, ismStore_GenId_t genId, ismStore_Handle_t *pOwnerHandle, uint64_t *pAttribute);
XAPI   int32_t ism_store_getNextPropOwner (ismStore_IteratorHandle *pIterator, ismStore_RecordType_t recordType, ismStore_GenId_t genId, ismStore_Handle_t *pOwnerHandle, uint64_t *pAttribute)
{
  double t;
  int32_t rc ; 
  t = ism_common_readTSC() ; 
  rc = ism_store_getNextPropOwner_(pIterator, recordType, genId, pOwnerHandle, pAttribute);
  recTime += ism_common_readTSC()-t ; 
  return rc ; 
    }
static int32_t ism_store_getNextPropOwner_(ismStore_IteratorHandle *pIterator, ismStore_RecordType_t recordType, ismStore_GenId_t genId, ismStore_Handle_t *pOwnerHandle, uint64_t *pAttribute)
#else
XAPI   int32_t ism_store_getNextPropOwner (ismStore_IteratorHandle *pIterator, ismStore_RecordType_t recordType, ismStore_GenId_t genId, ismStore_Handle_t *pOwnerHandle, uint64_t *pAttribute)
#endif
{
  ismStore_IteratorHandle  pIter ; 
  ismStore_GenId_t gid = genId , gind ; 
  ismStore_Handle_t handle ;
  char *genData ;
  int rc ; 

  if ( !(pIterator && pOwnerHandle && pAttribute) )
    {
    TRACE(1,"Bad arguments: function %s, pIterator %p , pOwnerHandle %p , pAttribute %p\n", __FUNCTION__, pIterator, pOwnerHandle,pAttribute);
    return ISMRC_NullArgument ;
  }

  pIter = *pIterator ; 
  if ( !pIter )
      {
    int j = T2T[recordType] ;
    ismStore_ownerByInd_t *Owners = &prpOwners[j]; 
    if (!Owners->Owners || !Owners->OwnersSize || Owners->OwnersInd >= Owners->OwnersSize )
    {
      return ISMRC_StoreNoMoreEntries ;
      }
    gind = allGens[gid-minGen].genInd ; 
    while ( Owners->OwnersInd < Owners->OwnersSize && ismSTORE_EXTRACT_GENID(Owners->Owners[Owners->OwnersInd]) < gind ) Owners->OwnersInd++ ; 
    if ( Owners->OwnersInd >= Owners->OwnersSize || ismSTORE_EXTRACT_GENID(Owners->Owners[Owners->OwnersInd]) > gind )
      {
      return ISMRC_StoreNoMoreEntries ;
      }

    if (!(genData = ism_store_getGen(ismSTORE_MGMT_GEN_ID,&rc)) )
      return rc ; 
    pIter = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,71),sizeof(ismStore_Iterator_t)) ; 
    if ( !pIter )
      return ISMRC_AllocateError ;
    memset(pIter,0,sizeof(ismStore_Iterator_t)) ; 
    pIter->type = ISM_STORE_ITERTYPE_OWNER; 
    pIter->genId = genId ; 
    pIter->genData = genData ; 
    pIter->gid0    = gind ; 
    pIter->Owners  = Owners ; 
    pIter->index   = Owners->OwnersInd ; 
    *pIterator = pIter ; 
  }

  if ( pIter && (pIter->genId != genId || pIter->type != ISM_STORE_ITERTYPE_OWNER) )
      {
    TRACE(1,"Bad arguments: function %s, pIter %p, pIter->genId %hu, genId %hu \n",
          __FUNCTION__, pIter, pIter->genId, genId);
    ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "pIterator");
    return ISMRC_ArgNotValid ;
      }

  if ( pIter->index < pIter->Owners->OwnersSize )
  {
    handle = pIter->Owners->Owners[pIter->index] ; 
    gind =  ismSTORE_EXTRACT_GENID(handle) ; 
    if ( gind == pIter->gid0 )
    {
      ismStore_memDescriptor_t *desc ;
      ismStore_Handle_t offset ; 
  
      genData = pIter->genData ; 
      offset = ismSTORE_EXTRACT_OFFSET(handle) ; 
      desc = (ismStore_memDescriptor_t *)(genData + offset) ;
      *pOwnerHandle = ismSTORE_BUILD_HANDLE(ismSTORE_MGMT_GEN_ID,offset) ;
      *pAttribute   = (uint64_t)desc->Attribute ; 
      pIter->index++ ;
      return ISMRC_OK ;
    }
  }

  ism_common_free(ism_memory_store_misc,pIter); 
  *pIterator = NULL ; 
  return ISMRC_StoreNoMoreEntries ;
}
/*---------------------------------------------------------------------------*/

static int ism_store_getRefOwners(ismStore_GenId_t gid)
  {
  int rc, i;
  register size_t DS ; 
  size_t memSize ; 
  ismStore_memGenInfo_t *gi ; 
  ismStore_memGenHeader_t *pGenHeader ;
  ismStore_memDescriptor_t *desc, *desc2 ; 
  ismStore_memReferenceChunk_t *ch ;
  ismStore_Handle_t owner, offset;
  double td;

  gi = allGens + (gid-minGen) ;
  if ( gi->ownersArraySize || gid == ismSTORE_MGMT_GEN_ID )
    return ISMRC_OK ; 
  if (!ism_store_getGen(gid,&rc) )
    return rc ; 
  td = su_sysTime() ;
  pGenHeader = (ismStore_memGenHeader_t *)gi->genData ;
  DS = pGenHeader->DescriptorStructSize ; 

  memset(gi->refOwners, 0, sizeof(gi->refOwners)) ; 
  if ( pGenHeader->CompactSizeBytes )
  {
    char *bptr = (char *)pGenHeader + pGenHeader->GranulePool[0].Offset ; 
    char *eptr = (char *)pGenHeader + pGenHeader->CompactSizeBytes ; 
    while ( bptr < eptr )
    {
      desc = (ismStore_memDescriptor_t *)bptr ; 
      bptr += ALIGNED(DS+desc->DataLength) ; 

      if ( desc->DataType != ismSTORE_DATATYPE_REFERENCES )
        continue ; 

      ch = (ismStore_memReferenceChunk_t *)((uintptr_t)desc+DS) ;
      owner = ch->OwnerHandle ; 
      offset= ismSTORE_EXTRACT_OFFSET(owner) ;
      desc2 = (ismStore_memDescriptor_t *)(ismStore_memGlobal.pStoreBaseAddress + offset) ;
      if ( ismSTORE_IS_SPLITITEM(desc2->DataType) )
      {
        ismStore_memSplitItem_t *si ;
        ismStore_memMgmtHeader_t *pMgmHeader;
        pMgmHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress;
        si = (ismStore_memSplitItem_t *)((uintptr_t)desc2+pMgmHeader->DescriptorStructSize) ;
        if ( si->Version == ch->OwnerVersion )
        {
          gi->ownersArraySize++ ; 
          gi->refOwners[T2T[desc2->DataType]].OwnersInd++ ; 
        }
      }
    }
  }
  else
  {
    uint64_t upto=0, blockSize=0 ;
    offset=0 ;
    i=0 ; 
    for(;;)
    {
      if ( offset >= upto )
      {
        if ( pGenHeader->PoolsCount > i && offset <= pGenHeader->GranulePool[i].Offset )
        {
          blockSize = pGenHeader->GranulePool[i].GranuleSizeBytes ; 
          offset    = pGenHeader->GranulePool[i].Offset ; 
          upto      = offset + pGenHeader->GranulePool[i].MaxMemSizeBytes ; 
          i++ ; 
        }
        else
        {
          break ; 
        }
      }
      desc = (ismStore_memDescriptor_t *)(gi->genData + offset) ;
      offset += blockSize ; 

      if ( desc->DataType != ismSTORE_DATATYPE_REFERENCES )
        continue ; 
  
      ch = (ismStore_memReferenceChunk_t *)((uintptr_t)desc+DS) ;
      owner = ch->OwnerHandle ; 
      desc2 = (ismStore_memDescriptor_t *)(ismStore_memGlobal.pStoreBaseAddress + ismSTORE_EXTRACT_OFFSET(owner)) ;
      if ( ismSTORE_IS_SPLITITEM(desc2->DataType) )
      {
        ismStore_memSplitItem_t *si ;
        ismStore_memMgmtHeader_t *pMgmHeader;
        pMgmHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress;
        si = (ismStore_memSplitItem_t *)((uintptr_t)desc2+pMgmHeader->DescriptorStructSize) ;
        if ( si->Version == ch->OwnerVersion )
        {
          gi->ownersArraySize++ ; 
          gi->refOwners[T2T[desc2->DataType]].OwnersInd++ ; 
        }
      }
    }
  }

  if (!gi->ownersArraySize )
    return ISMRC_OK ; 
    
  memSize = gi->ownersArraySize * sizeof(uint64_t) ; 
  gi->ownersArray = (uint64_t *)ism_store_getGenMem(memSize, 2, gid, &rc) ; 
  if ( !gi->ownersArray )
  {
    gi->ownersArraySize = 0 ; 
    memset(gi->refOwners, 0, sizeof(gi->refOwners)) ; 
    return ISMRC_AllocateError ;
  }

  for ( i=0, offset=0 ; i<ISM_STORE_NUM_REC_TYPES ; i++ )
  {
    if ( gi->refOwners[i].OwnersInd )
    {
      gi->refOwners[i].Owners = gi->ownersArray + offset ; 
      offset += gi->refOwners[i].OwnersInd ; 
    }
  }
  
  if ( pGenHeader->CompactSizeBytes )
  {
    char *bptr = (char *)pGenHeader + pGenHeader->GranulePool[0].Offset ; 
    char *eptr = (char *)pGenHeader + pGenHeader->CompactSizeBytes ; 
    while ( bptr < eptr )
    {
      desc = (ismStore_memDescriptor_t *)bptr ; 
      bptr += ALIGNED(DS+desc->DataLength) ; 

      if ( desc->DataType != ismSTORE_DATATYPE_REFERENCES )
        continue ; 

      ch = (ismStore_memReferenceChunk_t *)((uintptr_t)desc+DS) ;
      owner = ch->OwnerHandle ; 
      offset= ismSTORE_EXTRACT_OFFSET(owner) ;
      desc2 = (ismStore_memDescriptor_t *)(ismStore_memGlobal.pStoreBaseAddress + offset) ;
      if ( ismSTORE_IS_SPLITITEM(desc2->DataType) )
      {
        ismStore_memSplitItem_t *si ;
        ismStore_memMgmtHeader_t *pMgmHeader;
        pMgmHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress;
        si = (ismStore_memSplitItem_t *)((uintptr_t)desc2+pMgmHeader->DescriptorStructSize) ;
        if ( si->Version == ch->OwnerVersion )
        {
          ismStore_ownerByInd_t *pO = &gi->refOwners[T2T[desc2->DataType]] ; 
          pO->Owners[pO->OwnersSize++] = owner ; 
        }
      }
    }
  }
  else
  {
    uint64_t upto=0, blockSize=0 ;
    offset=0 ;
    i=0 ; 
    for(;;)
    {
      if ( offset >= upto )
      {
        if ( pGenHeader->PoolsCount > i && offset <= pGenHeader->GranulePool[i].Offset )
        {
          blockSize = pGenHeader->GranulePool[i].GranuleSizeBytes ; 
          offset    = pGenHeader->GranulePool[i].Offset ; 
          upto      = offset + pGenHeader->GranulePool[i].MaxMemSizeBytes ; 
          i++ ; 
        }
        else
        {
          break ; 
        }
      }
      desc = (ismStore_memDescriptor_t *)(gi->genData + offset) ;
      offset += blockSize ; 

      if ( desc->DataType != ismSTORE_DATATYPE_REFERENCES )
        continue ; 
  
      ch = (ismStore_memReferenceChunk_t *)((uintptr_t)desc+DS) ;
      owner = ch->OwnerHandle ; 
      desc2 = (ismStore_memDescriptor_t *)(ismStore_memGlobal.pStoreBaseAddress + ismSTORE_EXTRACT_OFFSET(owner)) ;
      if ( ismSTORE_IS_SPLITITEM(desc2->DataType) )
      {
        ismStore_memSplitItem_t *si ;
        ismStore_memMgmtHeader_t *pMgmHeader;
        pMgmHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress;
        si = (ismStore_memSplitItem_t *)((uintptr_t)desc2+pMgmHeader->DescriptorStructSize) ;
        if ( si->Version == ch->OwnerVersion )
        {
          ismStore_ownerByInd_t *pO = &gi->refOwners[T2T[desc2->DataType]] ; 
          pO->Owners[pO->OwnersSize++] = owner ; 
        }
      }
    }
  }

  recTimes[8] += su_sysTime() - td ; 
  return ISMRC_OK ; 
}

/*---------------------------------------------------------------------------*/

#if USE_REC_TIME
static int32_t ism_store_getNextRefOwner_(ismStore_IteratorHandle *pIterator, ismStore_RecordType_t recordType, ismStore_GenId_t genId, ismStore_Handle_t *pOwnerHandle);
XAPI   int32_t ism_store_getNextRefOwner (ismStore_IteratorHandle *pIterator, ismStore_RecordType_t recordType, ismStore_GenId_t genId, ismStore_Handle_t *pOwnerHandle)
{
  double t;
  int32_t rc ; 
  t = ism_common_readTSC() ; 
  rc = ism_store_getNextRefOwner_(pIterator, recordType, genId, pOwnerHandle) ; 
  recTime += ism_common_readTSC()-t ; 
  return rc ; 
}
static int32_t ism_store_getNextRefOwner_(ismStore_IteratorHandle *pIterator, ismStore_RecordType_t recordType, ismStore_GenId_t genId, ismStore_Handle_t *pOwnerHandle)
#else
XAPI   int32_t ism_store_getNextRefOwner (ismStore_IteratorHandle *pIterator, ismStore_RecordType_t recordType, ismStore_GenId_t genId, ismStore_Handle_t *pOwnerHandle)
#endif
{
  ismStore_IteratorHandle  pIter ; 
  ismStore_GenId_t gid = genId ; 
  ismStore_memGenInfo_t *gi ; 


  if ( !(pIterator && pOwnerHandle) )
  {
    TRACE(1,"Bad arguments: function %s, pIterator %p, pOwnerHandle %p\n",
          __FUNCTION__, pIterator, pOwnerHandle) ;
    return ISMRC_NullArgument ;
  }

  gi = allGens + (gid-minGen) ;
  pIter = *pIterator ; 
  if ( !pIter )
  {
    int rc;
    ismStore_ownerByInd_t *pO = &gi->refOwners[T2T[recordType]] ; 
    if ( (rc = ism_store_getRefOwners(gid)) != ISMRC_OK )
      return rc ; 
    if (!pO->OwnersSize )
      return ISMRC_StoreNoMoreEntries ;
    pIter = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,73),sizeof(ismStore_Iterator_t)) ; 
    if ( !pIter )
      return ISMRC_AllocateError ;
    memset(pIter,0,sizeof(ismStore_Iterator_t)) ; 
    pIter->type = ISM_STORE_ITERTYPE_OWNER; 
    pIter->genId = genId ; 
    pIter->Owners = pO ; 
    *pIterator = pIter ; 
  }

  if ( pIter && (pIter->genId != gid || pIter->type != ISM_STORE_ITERTYPE_OWNER) )
  {
    TRACE(1,"Bad arguments: function %s, pIter %p, pIter->genId %hu, genId %hu\n",
          __FUNCTION__, pIter, pIter->genId, genId);
    ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "pIterator");
    return ISMRC_ArgNotValid ;
  }

  if ( pIter->index < pIter->Owners->OwnersSize )
  {
    *pOwnerHandle = pIter->Owners->Owners[pIter->index++] ; 
    return ISMRC_OK ;
  }

  ism_common_free(ism_memory_store_misc,pIter); 
  *pIterator = NULL ; 
  return ISMRC_StoreNoMoreEntries ;
}
#else
XAPI   int32_t ism_store_getNextNewOwner (ismStore_IteratorHandle *pIterator, ismStore_RecordType_t recordType, ismStore_GenId_t genId, ismStore_Handle_t *pHandle, ismStore_Record_t *pRecord){return ISMRC_StoreNoMoreEntries;}
XAPI   int32_t ism_store_getNextPropOwner (ismStore_IteratorHandle *pIterator, ismStore_RecordType_t recordType, ismStore_GenId_t genId, ismStore_Handle_t *pOwnerHandle, uint64_t *pAttribute){return ISMRC_StoreNoMoreEntries;}
XAPI   int32_t ism_store_getNextRefOwner (ismStore_IteratorHandle *pIterator, ismStore_RecordType_t recordType, ismStore_GenId_t genId, ismStore_Handle_t *pOwnerHandle){return ISMRC_StoreNoMoreEntries;}
#endif
/*---------------------------------------------------------------------------*/

static int refOk(ismStore_memSplitItem_t *si, ismStore_memReference_t *ref, ismStore_Reference_t *pReference, ismStore_Handle_t *cache)
{                
  int iok=0 ; 
  if ( ref->RefHandle && pReference->OrderId >= si->MinActiveOrderId )
  {
    int i ; 
    ismStore_memRefGen_t *rg ;
    ismStore_memReferenceContext_t *rCtx = (ismStore_memReferenceContext_t *)si->pRefCtxt ;
    pReference->hRefHandle = ref->RefHandle ;
    pReference->Value      = ref->Value ;
    pReference->State      = ref->State ;
    iok = 1 ; 
    if ( (rg=rCtx->pRefGenState) && pReference->OrderId >= rg->LowestOrderId &&
                                    pReference->OrderId <= rg->HighestOrderId )
    {
      ismStore_memMgmtHeader_t *pMgmHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress ;
      ismStore_memDescriptor_t *desc ;
      ismStore_Handle_t handle = rg->hReferenceHead ; 
      if ( *cache )
        handle = *cache ; 
      else
      if (rCtx->pRFFingers && rCtx->pRFFingers->NumInUse)
      {
        ismStore_memRefStateFingers_t *pF = rCtx->pRFFingers ; 
        i = su_findGLB(pReference->OrderId, 0, pF->NumInUse-1, pF->BaseOid) ; 
        if ( i >= 0 )
        {
          desc = (ismStore_memDescriptor_t *)(ismStore_memGlobal.pStoreBaseAddress + ismSTORE_EXTRACT_OFFSET(pF->Handles[i]));
          ismStore_memRefStateChunk_t *stchk = (ismStore_memRefStateChunk_t *)((uintptr_t)desc+pMgmHeader->DescriptorStructSize) ;
          if (desc->DataType == ismSTORE_DATATYPE_REFSTATES &&
              stchk->OwnerHandle == rCtx->OwnerHandle &&
              stchk->OwnerVersion == rCtx->OwnerVersion &&
              pReference->OrderId >= stchk->BaseOrderId )
          {
             handle = pF->Handles[i];
          }
        }
      }
      for ( ; handle ; handle=desc->NextHandle )
      {
        desc = (ismStore_memDescriptor_t *)(ismStore_memGlobal.pStoreBaseAddress + ismSTORE_EXTRACT_OFFSET(handle)) ;
        ismStore_memRefStateChunk_t *stchk = (ismStore_memRefStateChunk_t *)((uintptr_t)desc+pMgmHeader->DescriptorStructSize) ;
        if ( pReference->OrderId < stchk->BaseOrderId )
          break ; 
        if ( pReference->OrderId <  stchk->BaseOrderId + stchk->RefStateCount )
        {
          i = pReference->OrderId - stchk->BaseOrderId ; 
          if ( stchk->RefStates[i] == ismSTORE_REFSTATE_DELETED )
            iok = 0 ; 
          else
          if ( stchk->RefStates[i] != ismSTORE_REFSTATE_NOT_VALID )
            pReference->State = stchk->RefStates[i] ;
          *cache = handle ; 
          break ; 
        }
      }
    }
  }
  return iok ; 
}

/*---------------------------------------------------------------------------*/

#if USE_REC_TIME
static int32_t ism_store_memGetNextReferenceForOwner_(ismStore_IteratorHandle *pIterator, ismStore_Handle_t hOwnerHandle, ismStore_GenId_t genId, ismStore_Handle_t    *pHandle, ismStore_Reference_t *pReference);
XAPI int32_t ism_store_memGetNextReferenceForOwner(ismStore_IteratorHandle *pIterator, ismStore_Handle_t hOwnerHandle, ismStore_GenId_t genId, ismStore_Handle_t    *pHandle, ismStore_Reference_t *pReference)
{
  double t;
  int32_t rc ; 
  t = ism_common_readTSC() ; 
  rc = ism_store_memGetNextReferenceForOwner_(pIterator, hOwnerHandle, genId, pHandle, pReference);
  recTime += ism_common_readTSC()-t ; 
  return rc ; 
}
static int32_t ism_store_memGetNextReferenceForOwner_(ismStore_IteratorHandle *pIterator, ismStore_Handle_t hOwnerHandle, ismStore_GenId_t genId, ismStore_Handle_t    *pHandle, ismStore_Reference_t *pReference)
#else
XAPI int32_t ism_store_memGetNextReferenceForOwner(ismStore_IteratorHandle *pIterator,
                                                ismStore_Handle_t hOwnerHandle,
                                                ismStore_GenId_t genId,
                                                ismStore_Handle_t    *pHandle,
                                                ismStore_Reference_t *pReference)
#endif
{
  int rc;
  register size_t DS ; 
  ismStore_memGenHeader_t  *pGenHeader;
  ismStore_IteratorHandle  pIter ; 
  ismStore_memDescriptor_t *desc ;
  ismStore_GenId_t gid = genId ; 
  char *genData ; 

  if ( !(pIterator && pHandle && pReference) )
    return ISMRC_NullArgument ;

  pIter = *pIterator ; 
  if ( !pIter )
  {
    int i;
    ismStore_Handle_t offset ;
    ismStore_memSplitItem_t *si ;
    pGenHeader = (ismStore_memGenHeader_t *)ismStore_memGlobal.pStoreBaseAddress ;
    DS = pGenHeader->DescriptorStructSize ; 
    desc = (ismStore_memDescriptor_t *)(ismStore_memGlobal.pStoreBaseAddress + ismSTORE_EXTRACT_OFFSET(hOwnerHandle)) ;
    si = (ismStore_memSplitItem_t *)((uintptr_t)desc+DS) ;
    if ( ismSTORE_IS_SPLITITEM(desc->DataType) )
    {
      ismStore_memReferenceChunk_t *rfchk ;
      ismStore_memGenInfo_t *gi = allGens + (gid-minGen) ; 
      ismStore_memReferenceContext_t *rCtx = (ismStore_memReferenceContext_t *)si->pRefCtxt ;
      ismStore_memRefGen_t *rg ;
      if ( !rCtx )
      {
        TRACE(1,"Failed to retrieve reference, because there is no reference context. Record type %u\n", desc->DataType);
        return ISMRC_Error ;
      }

      if (!(genData=ism_store_getGen(gid,&rc)) )
        return rc ;

      if ( rCtx->pRefGenLast )
        rg = rCtx->pRefGenLast ; 
      else
        rg = rCtx->pRefGenHead ;
      for ( ; rg ; rg=rg->Next )
      {
        ismStore_GenId_t ngid = ismSTORE_EXTRACT_GENID(rg->hReferenceHead) ; 
        ismStore_memGenInfo_t *ngi = allGens + (ngid-minGen) ; 
        if ( ngid == gid )
        {
          rCtx->pRefGenLast = rg ; 
          if (!ismSTORE_EXTRACT_OFFSET(rg->hReferenceHead) )
            rg = NULL ;
          break ;
        }
        else
        if ( gi->genInd < ngi->genInd )
        {
          if ( gi->genInd+1 == ngi->genInd )
            rCtx->pRefGenLast = rg ; 
          rg = NULL ; 
          break ; 
        }
        rCtx->pRefGenLast = rg ; 
      }
      if ( !rg )
        return ISMRC_StoreNoMoreEntries ;

      offset = ismSTORE_EXTRACT_OFFSET(rg->hReferenceHead) ;
      pGenHeader = (ismStore_memGenHeader_t *)genData ;
      DS = pGenHeader->DescriptorStructSize ;
      if ( pGenHeader->CompactSizeBytes )
      {
        if ( (rc = ism_store_getGenMap(gid)) != ISMRC_OK )
          return rc ;
        gi = allGens + (gid-minGen) ;
        if (!(desc = off2desc(gi, offset)) )
          return ISMRC_Error ;
      }
      else
        desc = (ismStore_memDescriptor_t *)(genData + offset) ;
      pIter = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,75),sizeof(ismStore_Iterator_t)) ;
      if ( !pIter )
        return ISMRC_AllocateError ;
      memset(pIter,0,sizeof(ismStore_Iterator_t)) ;
      pIter->type = ISM_STORE_ITERTYPE_REFQUE; 
      pIter->genId = genId ; 
      pIter->owner = hOwnerHandle ; 
      pIter->genData = genData ;  
      pIter->si      = si ;  
      pIter->offset    = offset ;
      pIter->index     = 0 ; 
      pIter->rg        = rg ; 
      pIter->rCtx      = rCtx ; 
      if ( pGenHeader->CompactSizeBytes )
      {
        pIter->bptr = (char *)desc ; 
        pIter->eptr = pIter->bptr ; 
      }
      pIter->setOff = pIter->offset ; 
      pIter->setNew = 1 ; 
      pIter->setMap = 0 ; 
      rfchk = (ismStore_memReferenceChunk_t *)((uintptr_t)desc+DS) ;
      for ( i=0 ; i < pGenHeader->PoolsCount ; i++ )
      {
        if ( offset < pGenHeader->GranulePool[i].Offset + pGenHeader->GranulePool[i].MaxMemSizeBytes )
        {
          pIter->blockSize = (pGenHeader->GranulePool[i].GranuleSizeBytes - DS - offsetof(ismStore_memReferenceChunk_t, References)) / sizeof(ismStore_memReference_t) ; 
          pIter->refShift = (rfchk->BaseOrderId % pIter->blockSize) * sizeof(ismStore_memReference_t) ; 
          break ; 
        }
      }
      *pIterator = pIter ; 
      if ( gi->state & 4 )
      {
        if ( rCtx->NextPruneOrderId > rfchk->BaseOrderId + rfchk->ReferenceCount )
             rCtx->NextPruneOrderId = rfchk->BaseOrderId + rfchk->ReferenceCount ;
      }
    }
    else
    {
      ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "hOwnerHandle") ; 
      return ISMRC_ArgNotValid ;
    }
  }

  if ( pIter && (pIter->genId != genId || pIter->owner != hOwnerHandle || pIter->type != ISM_STORE_ITERTYPE_REFQUE) )
  {
    ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "pIterator");
    return ISMRC_ArgNotValid ;
  }

  genData = pIter->genData ; 
  pGenHeader = (ismStore_memGenHeader_t *)genData ;
  DS = pGenHeader->DescriptorStructSize ; 

  if ( pGenHeader->CompactSizeBytes )
  {
    for(;;)
    {
      ismStore_memReference_t *ref ;
      ismStore_memReferenceChunk_t *rfchk ;

      desc = (ismStore_memDescriptor_t *)pIter->bptr ; 
      rfchk = (ismStore_memReferenceChunk_t *)((uintptr_t)desc+DS) ;
      if ( pIter->index >= rfchk->ReferenceCount )
      {
        ismStore_memGranulePool_t *pPool;
        pIter->bptr += ALIGNED(DS+desc->DataLength) ; 
        if (!desc->NextHandle )
        {
          pIter->rg->numRefs = pIter->refCount*100 ; 
          if ( pIter->setMap || (pIter->rg->HighestOrderId + pIter->blockSize - 1) / pIter->blockSize * pIter->blockSize > pIter->si->MinActiveOrderId )
          {
            pIter->setLen = pIter->bptr - pIter->eptr ; 
            ism_store_memSetGenMap(ismStore_memGlobal.pGensMap[gid], ismSTORE_BITMAP_LIVE_IDX, pIter->setOff, pIter->setLen);
          }
          else
            ism_store_removeRefGen(pIter) ; 
          if ( pIter->rCtx->NextPruneOrderId > pIter->rg->HighestOrderId + 1 )
               pIter->rCtx->NextPruneOrderId = pIter->rg->HighestOrderId + 1 ;
          ism_common_free(ism_memory_store_misc,pIter); 
          *pIterator = NULL ; 
          return ISMRC_StoreNoMoreEntries ;
        }
        desc = (ismStore_memDescriptor_t *)pIter->bptr ; 
        pPool = &pGenHeader->GranulePool[desc->PoolId] ; 
        pIter->offset = pPool->Offset + (size_t)desc->GranuleIndex * pPool->GranuleSizeBytes ; 
        pIter->index = 0 ; 
        rfchk = (ismStore_memReferenceChunk_t *)((uintptr_t)desc+DS) ;
        pIter->refShift = (rfchk->BaseOrderId % pIter->blockSize) * sizeof(ismStore_memReference_t) ; 
        continue ; 
      }
      ref = &rfchk->References[pIter->index] ; 
      *pHandle = ismSTORE_BUILD_HANDLE(gid,(uintptr_t)ref - (uintptr_t)desc + pIter->offset + pIter->refShift) ; 
      pReference->OrderId = rfchk->BaseOrderId + pIter->index ; 
      pIter->index++ ;
      if ( refOk(pIter->si, ref, pReference, &pIter->cache) )
      { 
        pIter->rg->HighestOrderId = pReference->OrderId ; 
        pIter->setMap = 1 ; 
        pIter->refCount++ ;
        return ISMRC_OK ;
      }
    }
  }
  else
  {
    for(;;)
    {
      ismStore_memReference_t *ref ;
      ismStore_memReferenceChunk_t *rfchk ;

      desc = (ismStore_memDescriptor_t *)(genData + pIter->offset) ;
      if ( pIter->setNew )
      {
        pIter->setLen += DS + desc->DataLength ; 
        pIter->setNew = 0 ; 
      }
      rfchk = (ismStore_memReferenceChunk_t *)((uintptr_t)desc+DS) ;
      if ( pIter->index >= rfchk->ReferenceCount )
      {
        if (!desc->NextHandle )
        {
          pIter->rg->numRefs = pIter->refCount*100 ; 
          if ( pIter->setMap || (pIter->rg->HighestOrderId + pIter->blockSize - 1) / pIter->blockSize * pIter->blockSize > pIter->si->MinActiveOrderId )
            ism_store_memSetGenMap(ismStore_memGlobal.pGensMap[gid], ismSTORE_BITMAP_LIVE_IDX, pIter->setOff, pIter->setLen);
          else
            ism_store_removeRefGen(pIter) ; 
          if ( pIter->rCtx->NextPruneOrderId > pIter->rg->HighestOrderId + 1 )
               pIter->rCtx->NextPruneOrderId = pIter->rg->HighestOrderId + 1 ;
          ism_common_free(ism_memory_store_misc,pIter); 
          *pIterator = NULL ; 
          return ISMRC_StoreNoMoreEntries ;
        }
        pIter->offset= ismSTORE_EXTRACT_OFFSET(desc->NextHandle) ; 
        pIter->index = 0 ; 
        pIter->setNew = 1 ; 
        continue ; 
      }
      ref = &rfchk->References[pIter->index] ; 
      *pHandle = ismSTORE_BUILD_HANDLE(gid,(uintptr_t)ref - (uintptr_t)genData) ; 
      pReference->OrderId = rfchk->BaseOrderId + pIter->index ; 
      pIter->index++ ;
      if ( refOk(pIter->si, ref, pReference, &pIter->cache) )
      { 
        pIter->rg->HighestOrderId = pReference->OrderId ; 
        pIter->setMap = 1 ; 
        pIter->refCount++ ;
        return ISMRC_OK ;
      }
    }
  }
}
/*---------------------------------------------------------------------------*/

#if USE_REC_TIME
static int32_t ism_store_memGetNextStateForOwner_(ismStore_IteratorHandle *pIterator, ismStore_Handle_t hOwnerHandle, ismStore_Handle_t    *pHandle, ismStore_StateObject_t *pState);
XAPI int32_t ism_store_memGetNextStateForOwner(ismStore_IteratorHandle *pIterator, ismStore_Handle_t hOwnerHandle, ismStore_Handle_t    *pHandle, ismStore_StateObject_t *pState)
{
  double t;
  int32_t rc ; 
  t = ism_common_readTSC() ; 
  rc = ism_store_memGetNextStateForOwner_(pIterator, hOwnerHandle, pHandle, pState);
  recTime += ism_common_readTSC()-t ; 
  return rc ; 
}
static int32_t ism_store_memGetNextStateForOwner_(ismStore_IteratorHandle *pIterator, ismStore_Handle_t hOwnerHandle, ismStore_Handle_t    *pHandle, ismStore_StateObject_t *pState)
#else
XAPI int32_t ism_store_memGetNextStateForOwner(ismStore_IteratorHandle *pIterator,
                                            ismStore_Handle_t hOwnerHandle,
                                            ismStore_Handle_t    *pHandle,
                                            ismStore_StateObject_t *pState)
#endif
{
  ismStore_memGenHeader_t  *pGenHeader;
  ismStore_IteratorHandle  pIter ; 
  ismStore_memDescriptor_t *desc ;
  register size_t DS;

  if ( !(pIterator && pHandle && pState) )
    return ISMRC_NullArgument ;

  pGenHeader = (ismStore_memGenHeader_t *)ismStore_memGlobal.pStoreBaseAddress ;
  DS = pGenHeader->DescriptorStructSize ; 
  pIter = *pIterator ; 
  if ( !pIter )
  {
    ismStore_memSplitItem_t *si ;
    ismStore_memStateContext_t *sCtx ; 
    desc = (ismStore_memDescriptor_t *)(ismStore_memGlobal.pStoreBaseAddress + ismSTORE_EXTRACT_OFFSET(hOwnerHandle)) ;
    if ( desc->DataType != ISM_STORE_RECTYPE_CLIENT )
    {
      ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "hOwnerHandle") ; 
      return ISMRC_ArgNotValid ;
    }
    si   = (ismStore_memSplitItem_t *)((uintptr_t)desc+DS) ;
    sCtx = (ismStore_memStateContext_t *)si->pStateCtxt ; 
    if ( !sCtx->hStateHead )
      return ISMRC_StoreNoMoreEntries ;
    pIter = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,78),sizeof(ismStore_Iterator_t)) ; 
    if ( !pIter )
      return ISMRC_AllocateError ;
    memset(pIter,0,sizeof(ismStore_Iterator_t)) ; 
    pIter->type = ISM_STORE_ITERTYPE_STATE; 
    pIter->owner = hOwnerHandle ; 
    pIter->handle    = sCtx->hStateHead; 
    pIter->index     = 0 ; 
    *pIterator = pIter ; 
  }
  if ( pIter && (pIter->owner != hOwnerHandle || pIter->type != ISM_STORE_ITERTYPE_STATE) )
  {
    ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "pIterator");
    return ISMRC_ArgNotValid ;
  }

  for(;;)
  {
    ismStore_memState_t *st ;
    desc = (ismStore_memDescriptor_t *)(ismStore_memGlobal.pStoreBaseAddress + ismSTORE_EXTRACT_OFFSET(pIter->handle)) ;
    ismStore_memStateChunk_t *chunk = (ismStore_memStateChunk_t *)((uintptr_t)desc+DS) ;
    if ( pIter->index >= ismStore_memGlobal.MgmtGen.MaxStatesPerGranule )
    {
      chunk->StatesCount = pIter->blockSize ; 
      if (!desc->NextHandle )
      {
        ism_common_free(ism_memory_store_misc,pIter); 
        *pIterator = NULL ; 
        return ISMRC_StoreNoMoreEntries ;
      }
      pIter->handle= desc->NextHandle ; 
      pIter->index = 0 ; 
      pIter->blockSize = 0 ; 
      continue ; 
    }
    st = &chunk->States[pIter->index] ; 
    if ( st->Flag == ismSTORE_STATEFLAG_VALID )
    {
      pState->Value      = st->Value ;
      *pHandle = ismSTORE_BUILD_HANDLE(ismSTORE_MGMT_GEN_ID,(uintptr_t)st - (uintptr_t)ismStore_memGlobal.pStoreBaseAddress) ;
      pIter->index++ ;
      pIter->blockSize++ ; 
      return ISMRC_OK ;
    }
    else
      pIter->index++ ;
  }
}
/*---------------------------------------------------------------------------*/

#if USE_REC_TIME
static int32_t ism_store_memReadRecord_(ismStore_Handle_t  handle, ismStore_Record_t *pRecord, uint8_t block);
XAPI int32_t ism_store_memReadRecord(ismStore_Handle_t  handle, ismStore_Record_t *pRecord, uint8_t block)
{
  double t;
  int32_t rc ; 
  t = ism_common_readTSC() ; 
  rc = ism_store_memReadRecord_(handle, pRecord, block);
  recTime += ism_common_readTSC()-t ; 
  return rc ; 
}
static int32_t ism_store_memReadRecord_(ismStore_Handle_t  handle, ismStore_Record_t *pRecord, uint8_t block)
#else
XAPI int32_t ism_store_memReadRecord(ismStore_Handle_t  handle, ismStore_Record_t *pRecord, uint8_t block)
#endif
{
  uint64_t offset ; 
  ismStore_memDescriptor_t *desc ;
  ismStore_memGenHeader_t *pGenHeader ;
  ismStore_GenId_t gid ; 
  ismStore_memGenInfo_t *gi ; 
  char *genData ; 
  int rc ; 

  if ( !(pRecord) )
  {
    TRACE(1,"Bad arguments: function %s, pRecord %p\n",__FUNCTION__, pRecord);
    return ISMRC_NullArgument ;
  }

  gid = ismSTORE_EXTRACT_GENID(handle) ; 
  offset = ismSTORE_EXTRACT_OFFSET(handle) ; 

  if ( !block )
  {
    int gok = ism_store_isGenIn(gid) ;
    if ( gok < 0 )
      return ISMRC_ArgNotValid ; 
    if ( gok == 0 )
      return ISMRC_WouldBlock ;
  }

  if (!(genData = ism_store_getGen(gid,&rc)) )
    return rc ; 

  gi = allGens + (gid-minGen) ;
  pGenHeader = (ismStore_memGenHeader_t *)genData ;
  if ( pGenHeader->CompactSizeBytes )
  {
    if ( (rc = ism_store_getGenMap(gid)) != ISMRC_OK )
      return rc ; 
    if (!(desc = off2desc(gi, offset)) )
      return ISMRC_ArgNotValid ; 
  }
  else
  {
    if ( offset >= gi->genSize )
      return ISMRC_ArgNotValid ; 
    desc = (ismStore_memDescriptor_t *)(genData + offset) ;
  }

  if ( (desc->DataType >= ISM_STORE_RECTYPE_MSG && desc->DataType <= ISM_STORE_RECTYPE_RPROP)  ||
       desc->DataType == ISM_STORE_RECTYPE_SERVER )
  {
    uint64_t l0 = pRecord->DataLength ; 
    uint64_t l1 = desc->TotalLength   ; 
    pRecord->Type      = (ismStore_RecordType_t)desc->DataType ; 
    pRecord->Attribute = desc->Attribute ; 
    pRecord->State     = desc->State ; 
    pRecord->DataLength = l1 ;
    if ( l0 < l1 )
      return ISMRC_StoreBufferTooSmall ;

    copyRecordData(genData, gid, offset, pRecord);
    return ISMRC_OK ;
  }
  TRACE(1,"Failed to read record for handle %lx, because the record type (%u) is not valid\n", handle, desc->DataType);

  ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "handle");
  return ISMRC_ArgNotValid ;
}
/*---------------------------------------------------------------------------*/

#if USE_REC_TIME
static int32_t internal_memReadReference_(ismStore_Handle_t  handle,ismStore_Reference_t *pReference, uint8_t block, uint8_t check, ismStore_Handle_t *pOwnerHandle, ismStore_RecordType_t *pOwnerRecType);
static int32_t internal_memReadReference(ismStore_Handle_t  handle,ismStore_Reference_t *pReference, uint8_t block, uint8_t check, ismStore_Handle_t *pOwnerHandle, ismStore_RecordType_t *pOwnerRecType)
{
  double t;
  int32_t rc ; 
  t = ism_common_readTSC() ; 
  rc = internal_memReadReference_(handle,pReference, block, check, pOwnerHandle, pOwnerRecType);
  recTime += ism_common_readTSC()-t ; 
  return rc ; 
}
static int32_t internal_memReadReference_(ismStore_Handle_t  handle,ismStore_Reference_t *pReference, uint8_t block, uint8_t check, ismStore_Handle_t *pOwnerHandle, ismStore_RecordType_t *pOwnerRecType)
#else
static int32_t internal_memReadReference(ismStore_Handle_t  handle,ismStore_Reference_t *pReference, uint8_t block, uint8_t check, ismStore_Handle_t *pOwnerHandle, ismStore_RecordType_t *pOwnerRecType)
#endif
{
  ismStore_memGenHeader_t  *pGenHeader;
  ismStore_memDescriptor_t *desc ;
  ismStore_memReferenceChunk_t *chunk ;
  ismStore_memSplitItem_t *si ;
  ismStore_Handle_t owner ; 
  ismStore_GenId_t gid ;
  ismStore_memReference_t *ref ;
  ismStore_Handle_t cache ; 
  uint64_t off  ;
  char *genData;
  int i , ind, rc;

  if ( !(pReference) )
  {
    ism_common_setErrorData(ISMRC_NullArgument, "%s", "pReference");
    return ISMRC_NullArgument ;
  }

  gid = ismSTORE_EXTRACT_GENID(handle) ; 
  off = ismSTORE_EXTRACT_OFFSET(handle) ; 

  if ( !block )
  {
    int gok = ism_store_isGenIn(gid) ;
    if ( gok < 0 )
    {
      ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "handle - genId");
      return ISMRC_ArgNotValid ; 
    }
    if ( gok == 0 )
      return ISMRC_WouldBlock ;
  }

  if (!(genData = ism_store_getGen(gid,&rc)) )
    return rc ; 

  pGenHeader = (ismStore_memGenHeader_t *)genData ;

  for ( i=0 ; i<pGenHeader->PoolsCount ; i++ )
  {
    if ( off >= pGenHeader->GranulePool[i].Offset &&
         off <  pGenHeader->GranulePool[i].Offset + pGenHeader->GranulePool[i].MaxMemSizeBytes )
    {
      uint64_t a,b,c ; 
      int indShift ; 
      a = off - pGenHeader->GranulePool[i].Offset ; 
      b = pGenHeader->GranulePool[i].GranuleSizeBytes ; 
      c = a  / b * b + pGenHeader->GranulePool[i].Offset ; 
      if ( pGenHeader->CompactSizeBytes )
      {
        ismStore_memGenInfo_t *gi ; 
        if ( (rc = ism_store_getGenMap(gid)) != ISMRC_OK )
          return rc ; 
        gi = allGens + (gid-minGen) ;
        if (!(desc = off2desc(gi, c)) )
          return ISMRC_Error ;
      }
      else
        desc = (ismStore_memDescriptor_t *)(genData + c) ;
      if ( (desc->DataType&(~ismSTORE_DATATYPE_NOT_PRIMARY)) != ismSTORE_DATATYPE_REFERENCES )
      {
        ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "handle - not reference block");
        return ISMRC_ArgNotValid ;
      }
      chunk = (ismStore_memReferenceChunk_t *)((uintptr_t)desc+pGenHeader->DescriptorStructSize) ;
      indShift = (pGenHeader->GranulePool[i].GranuleSizeBytes - pGenHeader->DescriptorStructSize - offsetof(ismStore_memReferenceChunk_t, References)) / sizeof(ismStore_memReference_t) ; 
      indShift = chunk->BaseOrderId % indShift ; 
      ind = (off - c - pGenHeader->DescriptorStructSize - offsetof(ismStore_memReferenceChunk_t, References)) / sizeof(ismStore_memReference_t) - indShift ; 
      ref = chunk->References + ind ; 
      pReference->OrderId = chunk->BaseOrderId + ind ; 
      break ; 
    }
  }
  if ( i>=pGenHeader->PoolsCount )
  {
    ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "handle - offset outside store");
    return ISMRC_ArgNotValid ;
  }

  owner = chunk->OwnerHandle ; 
  genData = ismStore_memGlobal.pStoreBaseAddress ;
  pGenHeader = (ismStore_memGenHeader_t *)genData ;
  desc = (ismStore_memDescriptor_t *)(genData + ismSTORE_EXTRACT_OFFSET(owner)) ;
  if ( ismSTORE_IS_SPLITITEM(desc->DataType) )
  {
    si = (ismStore_memSplitItem_t *)((uintptr_t)desc+pGenHeader->DescriptorStructSize) ;
    if ( si->Version == chunk->OwnerVersion )
    {
      cache = 0 ; 
      if ( UNLIKELY(check) )
      {
        if (!refOk(si, ref, pReference, &cache) )
        {
          ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "handle - reference not active");
          return ISMRC_ArgNotValid ; 
        }
      }
      else
      {
        pReference->hRefHandle = ref->RefHandle ;
        pReference->Value      = ref->Value ;
        pReference->State      = ref->State ;
      }

        if ( pOwnerHandle ) *pOwnerHandle = owner ; 
        if ( pOwnerRecType ) *pOwnerRecType = desc->DataType ; 
        return ISMRC_OK ;
      }
  }

  ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "handle - owner not valid");
  return ISMRC_ArgNotValid ; 
}

XAPI int32_t ism_store_memReadReference(ismStore_Handle_t  handle,ismStore_Reference_t *pReference)
{
  return internal_memReadReference(handle, pReference, 1, 1, NULL, NULL) ; 
}

XAPI int32_t ism_store_getReferenceInformation(ismStore_Handle_t handle, ismStore_Handle_t *pOwnerHandle, ismStore_RecordType_t *pOwnerRecType, uint64_t *pOrderId, uint8_t block)
{
  ismStore_Reference_t ref[1] ; 
  memset(ref,0,sizeof(ref));
  int rc = internal_memReadReference(handle, ref, block, 0, pOwnerHandle, pOwnerRecType);
  if ( rc == ISMRC_OK && pOrderId )
    *pOrderId = ref->OrderId ; 
  return rc ; 
}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
typedef struct
{
  char     *v[1] ; 
  uint32_t  l[1] ; 
} FragInfo ; 

typedef struct StoreRec_
{
  struct StoreRec_  *next ; 
  ismStore_Record_t rec[1] ; 
  ismStore_Handle_t handle ;
  FragInfo          fi[1] ;  
} StoreRec ; 

static const char *recName(ismStore_RecordType_t type)
{
  if ( type == ISM_STORE_RECTYPE_SERVER ) return "Server" ; 
  if ( type == ISM_STORE_RECTYPE_CLIENT ) return "Client" ; 
  if ( type == ISM_STORE_RECTYPE_QUEUE  ) return "Queue" ; 
  if ( type == ISM_STORE_RECTYPE_TOPIC  ) return "Topic" ; 
  if ( type == ISM_STORE_RECTYPE_SUBSC  ) return "Subscriber" ; 
  if ( type == ISM_STORE_RECTYPE_TRANS  ) return "Transaction" ; 
  if ( type == ISM_STORE_RECTYPE_BMGR   ) return "Bridge queue manager" ;
  if ( type == ISM_STORE_RECTYPE_REMSRV ) return "Remote Server" ;
  if ( type == ISM_STORE_RECTYPE_MSG    ) return "Message" ; 
  if ( type == ISM_STORE_RECTYPE_PROP   ) return "Property" ; 
  if ( type == ISM_STORE_RECTYPE_CPROP  ) return "Client Property" ; 
  if ( type == ISM_STORE_RECTYPE_QPROP  ) return "Queue Property" ; 
  if ( type == ISM_STORE_RECTYPE_TPROP  ) return "Topic Property" ; 
  if ( type == ISM_STORE_RECTYPE_SPROP  ) return "Subscriber Property" ; 
  if ( type == ISM_STORE_RECTYPE_BXR    ) return "Bridge XID" ; 
  if ( type == ISM_STORE_RECTYPE_RPROP  ) return "Remote Server Property" ; 
  return "Unknown" ; 
}
#if 0
static int isString(void *buf, size_t len) 
{
  uint8_t *c = (uint8_t *)buf , *e ; 
  e = c + len ; 
  for ( ; c<e ; c++ ) if (!isprint(*c) ) return !(*c) ;
  return 0;
}
#endif
static void initRecord(ismStore_Record_t *r, FragInfo *f)
{
  memset(r,0,sizeof(ismStore_Record_t)) ; 
  memset(f,0,sizeof(FragInfo)) ; 
  r->FragsCount = 1 ; 
  r->pFrags = f->v ; 
  r->pFragsLengths = f->l ; 
}

/*==============================================================*/
#define DUMP_MAX_REC_TYPE (ISM_STORE_RECTYPE_MAXVAL)

/*------------------------------------------------------*/

#if USE_NEXT_OWNER
XAPI int32_t ism_store_memDumpCB(ismPSTOREDUMPCALLBACK cbFunc, void *cbCtx)
{
  int32_t rc = ISMRC_OK;
  int iT ; 
  uint64_t svsz ; 
  ismStore_IteratorHandle genIter, recIter, refIter ; 
  ismStore_IteratorHandle ownIter;
  ismStore_GenId_t        genId ; 
  ismStore_Record_t rec[1], msg[1], prp[1];
  ismStore_Handle_t refHandle, recHandle; ; 
  ismStore_Reference_t ref[1] ; 
  ismStore_StateObject_t st[1] ; 
  StoreRec *first[DUMP_MAX_REC_TYPE], *last[DUMP_MAX_REC_TYPE], *sr ; 
  ismStore_RecordType_t recType;
  ismStore_DumpData_t dt[1];
  FragInfo fiR[1], fiM[1], fiP[1] ; 
  #if USE_REC_TIME
  double dumpTime, dumpStart, et, rt=0e0 ; 
  dumpStart = ism_common_readTSC() ; 
  #endif

  if (ismStore_memGlobal.State != ismSTORE_STATE_RECOVERY && ismStore_memGlobal.State != ismSTORE_STATE_ACTIVE)
  {
    TRACE(1, "Failed to dump the store's content, because the store's state (%d) is not valid\n", ismStore_memGlobal.State);
    return ISMRC_StoreNotAvailable;
  }

  initRecord(rec,fiR) ; 
  initRecord(msg,fiM) ; 
  initRecord(prp,fiP) ; 
  memset(first,0,sizeof(first)) ; 
  memset(last ,0,sizeof(last )) ; 
  genIter = NULL ; 
  for(;;)
  {
  #if USE_REC_TIME
    et = ism_common_readTSC() ; 
    rc = ism_store_memGetNextGenId(&genIter, &genId) ; 
    rt += ism_common_readTSC()-et ; 
  #else
    rc = ism_store_getNextGenId(&genIter, &genId) ; 
  #endif
    if ( rc == ISMRC_StoreNoMoreEntries )
      break ; 
    if ( rc != ISMRC_OK )
    {
      TRACE(1,"%s failed, because ism_store_getNextGenId failed with rc %d\n",__FUNCTION__, rc) ;
      goto exit ; 
    }
    dt->dataType = ISM_STORE_DUMP_GENID ; 
    dt->genId = genId ; 
    if ( (rc = cbFunc(dt, cbCtx)) != ISMRC_OK ) goto exit ;
    iT = T2T[ISM_STORE_RECTYPE_SERVER] ; 
    {
      recType = RT->rTypes[iT] ; 
      recIter = NULL ; 
      for(;;)
      {
        svsz = rec->DataLength ; 
      #if USE_REC_TIME
        et = ism_common_readTSC() ; 
        rc = ism_store_memGetNextRecordForType(&recIter, recType, genId, &recHandle, rec) ;
        rt += ism_common_readTSC()-et ; 
      #else
        rc = ism_store_memGetNextRecordForType(&recIter, recType, genId, &recHandle, rec) ;
      #endif
        if ( rc == ISMRC_StoreNoMoreEntries )
          break ; 
        if ( rc == ISMRC_StoreBufferTooSmall )
        {
          void *tmp ; 
          if ( !(tmp = ism_common_realloc(ISM_MEM_PROBE(ism_memory_store_misc,80),fiR->v[0], rec->DataLength)) )
          {
            TRACE(1,"%s failed to re-allocate %u bytes.\n",__FUNCTION__, rec->DataLength);
            rc = ISMRC_AllocateError ;
            goto exit ; 
          }
          fiR->l[0] = rec->DataLength ; 
          fiR->v[0] = tmp ; 
          continue ; 
        }
        if ( rc != ISMRC_OK )
        {
          TRACE(1,"%s failed, because ism_store_getNextRecordForType failed. Type %s, GenId %hu, rc %d\n",__FUNCTION__, recName(recType), genId, rc) ;
          goto exit ; 
        }
        dt->dataType = ISM_STORE_DUMP_REC4TYPE ; 
        dt->recType = recType ; 
        dt->handle = recHandle ; 
        dt->pRecord = rec ; 
        if ( (rc = cbFunc(dt, cbCtx)) != ISMRC_OK ) goto exit ;
        if (!(sr = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,81),sizeof(StoreRec))) )
        {
          TRACE(1,"%s failed to allocate memory of %lu bytes.\n", __FUNCTION__, sizeof(StoreRec));
          rc = ISMRC_AllocateError ;
          goto exit ; 
        } 
        memset(sr, 0, sizeof(StoreRec)) ; 
        initRecord(sr->rec, sr->fi) ; 
        memcpy(sr->rec, rec, sizeof(ismStore_Record_t)) ; 
        memcpy(sr->fi, fiR, sizeof(FragInfo)) ; 
        if (!(sr->fi->v[0] = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,82),rec->DataLength)) )
        {
          TRACE(1,"%s failed to allocate memory of %u bytes.\n", __FUNCTION__, rec->DataLength);
          rc = ISMRC_AllocateError ;
          goto exit ; 
        } 
        memcpy(sr->fi->v[0], fiR->v[0], rec->DataLength) ; 
        sr->handle = recHandle ; 
        if ( last[recType] )
          last[recType]->next = sr ; 
        else
          first[recType] = sr ; 
        last[recType] = sr ; 

        if ( rec->DataLength < svsz )
             rec->DataLength = svsz ;
      }
    }

    for ( iT=T2T[ISM_STORE_RECTYPE_CLIENT] ; iT<RT->f1st4gen ; iT++ )
          {
      recType = RT->rTypes[iT] ; 
      recIter = NULL ; 
            for(;;) 
            { 
        svsz = rec->DataLength ; 
             #if USE_REC_TIME
               et = ism_common_readTSC() ; 
        rc = ism_store_getNextNewOwner(&recIter, recType, genId, &recHandle, rec) ;
               rt += ism_common_readTSC()-et ; 
             #else
        rc = ism_store_getNextNewOwner(&recIter, recType, genId, &recHandle, rec) ;
             #endif
              if ( rc == ISMRC_StoreNoMoreEntries )
                break ; 
        if ( rc == ISMRC_StoreBufferTooSmall )
              {
          void *tmp ; 
          if ( !(tmp = ism_common_realloc(ISM_MEM_PROBE(ism_memory_store_misc,83),fiR->v[0], rec->DataLength)) )
          {
            TRACE(1,"%s failed to re-allocate %u bytes.\n",__FUNCTION__, rec->DataLength);
            rc = ISMRC_AllocateError ;
                goto exit ; 
              } 
          fiR->l[0] = rec->DataLength ; 
          fiR->v[0] = tmp ; 
          continue ; 
          }
        if ( rc != ISMRC_OK )
        {
          TRACE(1,"%s failed, because ism_store_getNextNewOwner failed. GenId %hu, rc %d\n",__FUNCTION__, genId, rc) ;
          goto exit ; 
        }
        dt->dataType = ISM_STORE_DUMP_REC4TYPE ; 
        dt->recType = recType ; 
        dt->handle = recHandle ; 
        dt->pRecord = rec ; 
        if ( (rc = cbFunc(dt, cbCtx)) != ISMRC_OK ) goto exit ;
        if (!(sr = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,84),sizeof(StoreRec))) )
        {
          TRACE(1,"%s failed to allocate memory of %lu bytes.\n", __FUNCTION__, sizeof(StoreRec));
          rc = ISMRC_AllocateError ;
          goto exit ; 
        } 
        memset(sr, 0, sizeof(StoreRec)) ; 
        initRecord(sr->rec, sr->fi) ; 
        memcpy(sr->rec, rec, sizeof(ismStore_Record_t)) ; 
        memcpy(sr->fi, fiR, sizeof(FragInfo)) ; 
        if (!(sr->fi->v[0] = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,85),rec->DataLength)) )
        {
          TRACE(1,"%s failed to allocate memory of %u bytes.\n", __FUNCTION__, rec->DataLength);
          rc = ISMRC_AllocateError ;
          goto exit ; 
        } 
        memcpy(sr->fi->v[0], fiR->v[0], rec->DataLength) ; 
        sr->handle = recHandle ; 
        if ( last[recType] )
          last[recType]->next = sr ; 
        else
          first[recType] = sr ; 
        last[recType] = sr ; 
  
        if ( rec->DataLength < svsz )
             rec->DataLength = svsz ;
  
        if ( recType == ISM_STORE_RECTYPE_CLIENT )
        {
        //for ( sr=first[recType] ; sr ; sr=sr->next )
          {
            refIter = NULL ; 
            for(;;) 
            { 
             #if USE_REC_TIME
               et = ism_common_readTSC() ; 
              rc = ism_store_memGetNextStateForOwner(&refIter, sr->handle, &refHandle, st) ; 
               rt += ism_common_readTSC()-et ; 
             #else
              rc = ism_store_getNextStateForOwner(&refIter, sr->handle, &refHandle, st) ; 
             #endif
              if ( rc == ISMRC_StoreNoMoreEntries )
                break ; 
              if ( rc != ISMRC_OK )
              {
                TRACE(1,"%s failed, because ism_store_getNextStateForOwner failed. Type %s, GenId %hu, rc %d\n",__FUNCTION__, recName(recType), genId, rc);
                goto exit ; 
              } 
              dt->dataType = ISM_STORE_DUMP_STATE4OWNER ; 
              dt->handle = refHandle ; 
              dt->owner  = sr->handle ; 
              dt->pState = st ; 
              if ( (rc = cbFunc(dt, cbCtx)) != ISMRC_OK ) goto exit ;
            } 
          }
        }
      }
      ownIter = NULL ; 
      for(;;)
      {
        uint64_t attr;
       #if USE_REC_TIME
         et = ism_common_readTSC() ; 
        rc = ism_store_getNextPropOwner(&ownIter, recType, genId, &recHandle, &attr); 
         rt += ism_common_readTSC()-et ; 
       #else
        rc = ism_store_getNextPropOwner(&ownIter, recType, genId, &recHandle, &attr); 
       #endif
        if ( rc == ISMRC_StoreNoMoreEntries )
          break ; 
        if ( rc != ISMRC_OK )
        {
          TRACE(1,"%s failed, because ism_store_getNextPropOwner failed. GenId %hu, rc %d\n",__FUNCTION__, genId, rc);
          goto exit ; 
        } 
        if ( hasProp[T2T[recType]] ) 
        {
          ismStore_GenId_t gid;
          if ( ism_store_getGenIdOfHandle((ismStore_Handle_t)attr, &gid) == ISMRC_OK && gid == genId )
          {
            svsz = prp->DataLength ; 
           #if USE_REC_TIME
             et = ism_common_readTSC() ; 
            rc = ism_store_memReadRecord((ismStore_Handle_t)attr, prp,1) ; 
             rt += ism_common_readTSC()-et ; 
           #else
            rc = ism_store_readRecord((ismStore_Handle_t)attr, prp,1) ; 
           #endif
            if ( rc == ISMRC_StoreBufferTooSmall )
            {
              void *tmp ; 
              if (!(tmp = ism_common_realloc(ISM_MEM_PROBE(ism_memory_store_misc,86),fiP->v[0], prp->DataLength)) )
              {
                TRACE(1,"%s failed to allocate memory of %u bytes.\n",__FUNCTION__, prp->DataLength);
                rc = ISMRC_AllocateError ;
                goto exit ; 
              }
              fiP->l[0] = prp->DataLength ; 
              fiP->v[0] = tmp ; 
             #if USE_REC_TIME
               et = ism_common_readTSC() ; 
              rc = ism_store_memReadRecord((ismStore_Handle_t)attr, prp,1) ; 
               rt += ism_common_readTSC()-et ; 
             #else
              rc = ism_store_readRecord((ismStore_Handle_t)attr, prp,1) ; 
             #endif
            }
            if ( rc == ISMRC_OK && 
                 ((prp->Type >= ISM_STORE_RECTYPE_PROP && prp->Type <= ISM_STORE_RECTYPE_SPROP) || prp->Type == ISM_STORE_RECTYPE_RPROP) )
            {
              dt->dataType = ISM_STORE_DUMP_PROP ; 
              dt->handle = (ismStore_Handle_t)attr ; 
              dt->owner  = recHandle ; 
              dt->pRecord = prp ; 
              if ( (rc = cbFunc(dt, cbCtx)) != ISMRC_OK ) goto exit ;
            }
            if ( prp->DataLength < svsz )
                 prp->DataLength = svsz ;
          } 
        }
      }
      ownIter = NULL ; 
      for(;;)
      {
       #if USE_REC_TIME
         et = ism_common_readTSC() ; 
        rc = ism_store_getNextRefOwner(&ownIter, recType, genId, &recHandle); 
         rt += ism_common_readTSC()-et ; 
       #else
        rc = ism_store_getNextRefOwner(&ownIter, recType, genId, &recHandle); 
       #endif
        if ( rc == ISMRC_StoreNoMoreEntries )
          break ; 
        if ( rc != ISMRC_OK )
        {
          TRACE(1,"%s failed, because ism_store_getNextRefOwner failed. GenId %hu, rc %d\n",__FUNCTION__, genId, rc);
          goto exit ; 
        } 
        refIter = NULL ; 
        for(;;) 
        { 
         #if USE_REC_TIME
           et = ism_common_readTSC() ; 
          rc = ism_store_memGetNextReferenceForOwner(&refIter, recHandle, genId, &refHandle, ref) ; 
           rt += ism_common_readTSC()-et ; 
         #else
          rc = ism_store_getNextReferenceForOwner(&refIter, recHandle, genId, &refHandle, ref) ; 
         #endif
          if ( rc == ISMRC_StoreNoMoreEntries )
            break ; 
          if ( rc != ISMRC_OK )
          {
            TRACE(1,"%s failed, because ism_store_getNextReferenceForOwner failed. Type %s %d, GenId %hu, rc %d\n",__FUNCTION__, recName(recType), recType, genId, rc);
            goto exit ; 
          } 
          dt->dataType = ISM_STORE_DUMP_REF4OWNER ; 
          dt->handle = refHandle ; 
          dt->owner  = recHandle ; 
          dt->pReference = ref ; 
          dt->readRefHandle = 0 ; 
          if ( (rc = cbFunc(dt, cbCtx)) != ISMRC_OK ) goto exit ;
          if (ref->hRefHandle && dt->readRefHandle )
          {
            if ( recType == ISM_STORE_RECTYPE_TRANS )
            {
            #if USE_REC_TIME
              et = ism_common_readTSC() ; 
              rc = ism_store_memReadReference(ref->hRefHandle, ref) ; 
              rt += ism_common_readTSC()-et ; 
            #else
              rc = ism_store_memReadReference(ref->hRefHandle, ref) ; 
            #endif
              if ( rc != ISMRC_OK )
              {
                TRACE(1,"%s failed, because ism_store_memReadReference failed. Handle 0x%lx, rc %d\n",__FUNCTION__, ref->hRefHandle, rc);
                goto exit ; 
              } 
              dt->dataType = ISM_STORE_DUMP_REF ; 
              dt->handle = refHandle ; 
              dt->owner  = recHandle ; 
              dt->pReference = ref ; 
              dt->readRefHandle = 0 ; 
              if ( (rc = cbFunc(dt, cbCtx)) != ISMRC_OK ) goto exit ;
            }
            if (ref->hRefHandle && dt->readRefHandle )
            {
              svsz = msg->DataLength ; 
              msg->Type = ISM_STORE_RECTYPE_MSG; 
            #if USE_REC_TIME
              et = ism_common_readTSC() ; 
              rc = ism_store_memReadRecord(ref->hRefHandle, msg,1);
              rt += ism_common_readTSC()-et ; 
            #else
              rc = ism_store_readRecord(ref->hRefHandle, msg,1);
            #endif
              if ( rc == ISMRC_StoreBufferTooSmall )
              {
                void *tmp ; 
                if (!(tmp = ism_common_realloc(ISM_MEM_PROBE(ism_memory_store_misc,87),fiM->v[0], msg->DataLength)) )
                {
                  TRACE(1,"%s failed to allocate memory of %u bytes.\n",__FUNCTION__, msg->DataLength);
                  rc = ISMRC_AllocateError ;
                  goto exit ; 
                }
                fiM->l[0] = msg->DataLength ; 
                fiM->v[0] = tmp ; 
               #if USE_REC_TIME
                 et = ism_common_readTSC() ; 
                 rc = ism_store_memReadRecord(ref->hRefHandle, msg,1);
                 rt += ism_common_readTSC()-et ; 
               #else
                 rc = ism_store_readRecord(ref->hRefHandle, msg,1);
               #endif
              }
              if ( rc == ISMRC_OK )
              {
                dt->dataType = ISM_STORE_DUMP_MSG ; 
                dt->handle = ref->hRefHandle ; 
                dt->owner  = recHandle ; 
                dt->pRecord = msg ; 
                if ( (rc = cbFunc(dt, cbCtx)) != ISMRC_OK ) goto exit ;
              }
              else 
              { 
                TRACE(1,"%s failed, because ism_store_readRecord failed. Handle 0x%lx, Owner 0x%lx, Oid %lu, rc %d\n",__FUNCTION__, ref->hRefHandle, recHandle, ref->OrderId, rc) ;
               #if 0
                goto exit ; 
               #elif 0
                {
                int i;
                ismStore_memGenInfo_t *gi ; 
                ismStore_memGenHeader_t  *pGenHeader;
                ismStore_memDescriptor_t *desc ;
                ismStore_memReferenceChunk_t *chunk ; 
                ismStore_GenId_t gid ; 
                ismStore_Handle_t off ; 
                char *genData ; 
                gid = ismSTORE_EXTRACT_GENID(refHandle) ; 
                off = ismSTORE_EXTRACT_OFFSET(refHandle) ; 
                gi = allGens + (gid-minGen) ;
                desc = (ismStore_memDescriptor_t *)(gi->genData + (off/1024*1024)) ; 
                chunk =(ismStore_memReferenceChunk_t *)(desc+1) ; 
                TRACE(1,"_bad_ Desc: TotalLength=%lu , Attribute=%lu, State=%lu, NextHandle=%lx, DataLength=%u, DataType=%x, PoolId=%u, Pad=%u\n",
                                desc->TotalLength,desc->Attribute,desc->State,desc->NextHandle,desc->DataLength,desc->DataType,desc->PoolId,desc->Pad[0]) ; 
                TRACE(1,"_bad_ Chunk: OwnerHandle=%lx, BaseOrderId=%lu, ReferenceCount=%u, Pad=%u %u %u %u\n",
                               chunk->OwnerHandle,chunk->BaseOrderId,chunk->ReferenceCount,chunk->Pad[0],chunk->Pad[1],chunk->Pad[2],chunk->Pad[3]);
                for ( i=0 ; i<60 ; i++ )
                {
                  ismStore_memReference_t *ref = &chunk->References[i] ; 
                  TRACE(1,"_bad_ Ref: i=%d, RefHandle=%lx, Value=%u, State=%u, Pad=%u %u %u\n",
                        i, ref->RefHandle, ref->Value, ref->State, ref->Pad[0],ref->Pad[1],ref->Pad[2]);
                }
                }
               #endif
              } 
              if ( msg->DataLength < svsz )
                   msg->DataLength = svsz ;
            } 
          } 
        } 
      }
    }
  }
  rc = ISMRC_OK ;
 exit:
  for ( recType=ISM_STORE_RECTYPE_SERVER ; recType<ISM_STORE_RECTYPE_MSG ; recType++ )
  {
    while ( first[recType] )
    {
      sr = first[recType] ;
      first[recType] = sr->next ; 
      if ( sr->fi->v[0] ) ism_common_free(ism_memory_store_misc,sr->fi->v[0]) ;
      ism_common_free(ism_memory_store_misc,sr) ; 
    }
  }
  if ( fiR->v[0] ) ism_common_free(ism_memory_store_misc,fiR->v[0]) ; 
  if ( fiM->v[0] ) ism_common_free(ism_memory_store_misc,fiM->v[0]) ; 
  if ( fiP->v[0] ) ism_common_free(ism_memory_store_misc,fiP->v[0]) ; 
 #if USE_REC_TIME
  dumpTime = ism_common_readTSC() -dumpStart - rt ; 
  TRACE(1,"%s time: %f (rt=%f)\n",__FUNCTION__,dumpTime,rt);
 #endif
  return rc ; 
}
#else
XAPI int32_t ism_store_memDumpCB(ismPSTOREDUMPCALLBACK cbFunc, void *cbCtx)
{
  int32_t rc = ISMRC_OK;
  int iT, nT ; 
  uint64_t svsz ; 
  ismStore_IteratorHandle genIter, recIter, refIter;
  ismStore_GenId_t        genId ; 
  ismStore_Record_t rec[1], msg[1], prp[1];
  ismStore_Handle_t refHandle, recHandle; ; 
  ismStore_Reference_t ref[1] ; 
  ismStore_StateObject_t st[1] ; 
  StoreRec *first[DUMP_MAX_REC_TYPE], *last[DUMP_MAX_REC_TYPE], *sr ; 
  ismStore_RecordType_t recType;
  ismStore_DumpData_t dt[1];
  FragInfo fiR[1], fiM[1], fiP[1] ; 
  #if USE_REC_TIME
  double dumpTime, dumpStart, et, rt=0e0 ; 
  dumpStart = ism_common_readTSC() ; 
  #endif

  if (ismStore_memGlobal.State != ismSTORE_STATE_RECOVERY && ismStore_memGlobal.State != ismSTORE_STATE_ACTIVE)
  {
    TRACE(1, "Failed to dump the store's content, because the store's state (%d) is not valid\n", ismStore_memGlobal.State);
    return ISMRC_StoreNotAvailable;
  }

  initRecord(rec,fiR) ; 
  initRecord(msg,fiM) ; 
  initRecord(prp,fiP) ; 
  memset(first,0,sizeof(first)) ; 
  memset(last ,0,sizeof(last )) ; 
  genIter = NULL ; 
  for(;;)
  {
  #if USE_REC_TIME
    et = ism_common_readTSC() ; 
    rc = ism_store_memGetNextGenId(&genIter, &genId) ; 
    rt += ism_common_readTSC()-et ; 
  #else
    rc = ism_store_getNextGenId(&genIter, &genId) ; 
  #endif
    if ( rc == ISMRC_StoreNoMoreEntries )
      break ; 
    if ( rc != ISMRC_OK )
    {
      TRACE(1,"%s failed, because ism_store_getNextGenId failed with rc %d\n",__FUNCTION__, rc) ;
      goto exit ; 
    }
    iT = 0 ; 
    nT = RT->f1st4gen ; 
    dt->dataType = ISM_STORE_DUMP_GENID ; 
    dt->genId = genId ; 
    if ( (rc = cbFunc(dt, cbCtx)) != ISMRC_OK ) goto exit ;
    for ( ; iT<nT ; iT++ )
 // for ( recType=ISM_STORE_RECTYPE_SERVER ; recType<ISM_STORE_RECTYPE_MSG ; recType++ )
    {
      recType = RT->rTypes[iT] ; 
      dt->recType = recType ; 
      dt->dataType = ISM_STORE_DUMP_REC4TYPE ; 
      recIter = NULL ; 
      for(;;)
      {
        svsz = rec->DataLength ; 
      #if USE_REC_TIME
        et = ism_common_readTSC() ; 
        rc = ism_store_memGetNextRecordForType(&recIter, recType, genId, &recHandle, rec) ;
        rt += ism_common_readTSC()-et ; 
      #else
        rc = ism_store_memGetNextRecordForType(&recIter, recType, genId, &recHandle, rec) ;
      #endif
        if ( rc == ISMRC_StoreNoMoreEntries )
          break ; 
        if ( rc == ISMRC_StoreBufferTooSmall )
        {
          void *tmp ; 
          if ( !(tmp = ism_common_realloc(ISM_MEM_PROBE(ism_memory_store_misc,93),fiR->v[0], rec->DataLength)) )
          {
            TRACE(1,"%s failed to re-allocate %u bytes.\n",__FUNCTION__, rec->DataLength);
            rc = ISMRC_AllocateError ;
            goto exit ; 
          }
          fiR->l[0] = rec->DataLength ; 
          fiR->v[0] = tmp ; 
          continue ; 
        }
        if ( rc != ISMRC_OK )
        {
          TRACE(1,"%s failed, because ism_store_getNextRecordForType failed. Type %s, GenId %hu, rc %d\n",__FUNCTION__, recName(recType), genId, rc) ;
          goto exit ; 
        }
        dt->dataType = ISM_STORE_DUMP_REC4TYPE ; 
        dt->recType = recType ; 
        dt->handle = recHandle ; 
        dt->pRecord = rec ; 
        if ( (rc = cbFunc(dt, cbCtx)) != ISMRC_OK ) goto exit ;
        if (!(sr = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,94),sizeof(StoreRec))) )
        {
          TRACE(1,"%s failed to allocate memory of %lu bytes.\n", __FUNCTION__, sizeof(StoreRec));
          rc = ISMRC_AllocateError ;
          goto exit ; 
        } 
        memset(sr, 0, sizeof(StoreRec)) ; 
        initRecord(sr->rec, sr->fi) ; 
        memcpy(sr->rec, rec, sizeof(ismStore_Record_t)) ; 
        memcpy(sr->fi, fiR, sizeof(FragInfo)) ; 
        if (!(sr->fi->v[0] = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,95),rec->DataLength)) )
        {
          TRACE(1,"%s failed to allocate memory of %u bytes.\n", __FUNCTION__, rec->DataLength);
          rc = ISMRC_AllocateError ;
          goto exit ; 
        } 
        memcpy(sr->fi->v[0], fiR->v[0], rec->DataLength) ; 
        sr->handle = recHandle ; 
        if ( last[recType] )
          last[recType]->next = sr ; 
        else
          first[recType] = sr ; 
        last[recType] = sr ; 

        if ( rec->DataLength < svsz )
             rec->DataLength = svsz ;

        if ( recType == ISM_STORE_RECTYPE_CLIENT )
        {
        //for ( sr=first[recType] ; sr ; sr=sr->next )
          {
            refIter = NULL ; 
            for(;;) 
            { 
             #if USE_REC_TIME
               et = ism_common_readTSC() ; 
              rc = ism_store_memGetNextStateForOwner(&refIter, sr->handle, &refHandle, st) ; 
               rt += ism_common_readTSC()-et ; 
             #else
              rc = ism_store_getNextStateForOwner(&refIter, sr->handle, &refHandle, st) ; 
             #endif
              if ( rc == ISMRC_StoreNoMoreEntries )
                break ; 
              if ( rc != ISMRC_OK )
              {
                TRACE(1,"%s failed, because ism_store_getNextStateForOwner failed. Type %s, GenId %hu, rc %d\n",__FUNCTION__, recName(recType), genId, rc);
                goto exit ; 
              } 
              dt->dataType = ISM_STORE_DUMP_STATE4OWNER ; 
              dt->handle = refHandle ; 
              dt->owner  = sr->handle ; 
              dt->pState = st ; 
              if ( (rc = cbFunc(dt, cbCtx)) != ISMRC_OK ) goto exit ;
            } 
          }
        }

      }
      if ( ismSTORE_IS_SPLITITEM(recType) )
      {
        for ( sr=first[recType] ; sr ; sr=sr->next )
        {
          refIter = NULL ; 
          for(;;) 
          { 
           #if USE_REC_TIME
             et = ism_common_readTSC() ; 
            rc = ism_store_memGetNextReferenceForOwner(&refIter, sr->handle, genId, &refHandle, ref) ; 
             rt += ism_common_readTSC()-et ; 
           #else
            rc = ism_store_getNextReferenceForOwner(&refIter, sr->handle, genId, &refHandle, ref) ; 
           #endif
            if ( rc == ISMRC_StoreNoMoreEntries )
              break ; 
            if ( rc != ISMRC_OK )
            {
              TRACE(1,"%s failed, because ism_store_getNextReferenceForOwner failed. Type %s %d, GenId %hu, rc %d\n",__FUNCTION__, recName(recType), recType, genId, rc);
              goto exit ; 
            } 
            dt->dataType = ISM_STORE_DUMP_REF4OWNER ; 
            dt->handle = refHandle ; 
            dt->owner  = sr->handle ; 
            dt->pReference = ref ; 
            dt->readRefHandle = 0 ; 
            if ( (rc = cbFunc(dt, cbCtx)) != ISMRC_OK ) goto exit ;
            if (ref->hRefHandle && dt->readRefHandle )
            {
              if ( recType == ISM_STORE_RECTYPE_TRANS )
              {
              #if USE_REC_TIME
                et = ism_common_readTSC() ; 
                rc = ism_store_memReadReference(ref->hRefHandle, ref) ; 
                rt += ism_common_readTSC()-et ; 
              #else
                rc = ism_store_memReadReference(ref->hRefHandle, ref) ; 
              #endif
                if ( rc != ISMRC_OK )
                {
                  TRACE(1,"%s failed, because ism_store_memReadReference failed. Handle 0x%lx, rc %d\n",__FUNCTION__, ref->hRefHandle, rc);
                  goto exit ; 
                } 
                dt->dataType = ISM_STORE_DUMP_REF ; 
                dt->handle = refHandle ; 
                dt->owner  = sr->handle ; 
                dt->pReference = ref ; 
                dt->readRefHandle = 0 ; 
                if ( (rc = cbFunc(dt, cbCtx)) != ISMRC_OK ) goto exit ;
              }
              if (ref->hRefHandle && dt->readRefHandle )
              {
                svsz = msg->DataLength ; 
                msg->Type = ISM_STORE_RECTYPE_MSG; 
              #if USE_REC_TIME
                et = ism_common_readTSC() ; 
                rc = ism_store_memReadRecord(ref->hRefHandle, msg,1);
                rt += ism_common_readTSC()-et ; 
              #else
                rc = ism_store_readRecord(ref->hRefHandle, msg,1);
              #endif
                if ( rc == ISMRC_StoreBufferTooSmall )
                {
                  void *tmp ; 
                  if (!(tmp = ism_common_realloc(ISM_MEM_PROBE(ism_memory_store_misc,96),fiM->v[0], msg->DataLength)) )
                  {
                    TRACE(1,"%s failed to allocate memory of %u bytes.\n",__FUNCTION__, msg->DataLength);
                    rc = ISMRC_AllocateError ;
                    goto exit ; 
                  }
                  fiM->l[0] = msg->DataLength ; 
                  fiM->v[0] = tmp ; 
                 #if USE_REC_TIME
                   et = ism_common_readTSC() ; 
                   rc = ism_store_memReadRecord(ref->hRefHandle, msg,1);
                   rt += ism_common_readTSC()-et ; 
                 #else
                   rc = ism_store_readRecord(ref->hRefHandle, msg,1);
                 #endif
                }
                if ( rc == ISMRC_OK )
                {
                  dt->dataType = ISM_STORE_DUMP_MSG ; 
                  dt->handle = ref->hRefHandle ; 
                  dt->owner  = sr->handle ; 
                  dt->pRecord = msg ; 
                  if ( (rc = cbFunc(dt, cbCtx)) != ISMRC_OK ) goto exit ;
                }
                else 
                { 
                  TRACE(1,"%s failed, because ism_store_readRecord failed. Handle 0x%lx, Owner 0x%lx, Oid %lu, rc %d\n",__FUNCTION__, ref->hRefHandle, sr->handle, ref->OrderId, rc) ;
                 #if 0
                  goto exit ; 
                 #elif 0
                  {
                  int i;
                  ismStore_memGenInfo_t *gi ; 
                  ismStore_memGenHeader_t  *pGenHeader;
                  ismStore_memDescriptor_t *desc ;
                  ismStore_memReferenceChunk_t *chunk ; 
                  ismStore_GenId_t gid ; 
                  ismStore_Handle_t off ; 
                  char *genData ; 
                  gid = ismSTORE_EXTRACT_GENID(refHandle) ; 
                  off = ismSTORE_EXTRACT_OFFSET(refHandle) ; 
                  gi = allGens + (gid-minGen) ;
                  desc = (ismStore_memDescriptor_t *)(gi->genData + (off/1024*1024)) ; 
                  chunk =(ismStore_memReferenceChunk_t *)(desc+1) ; 
                  TRACE(1,"_bad_ Desc: TotalLength=%lu , Attribute=%lu, State=%lu, NextHandle=%lx, DataLength=%u, DataType=%x, PoolId=%u, Pad=%u\n",
                                  desc->TotalLength,desc->Attribute,desc->State,desc->NextHandle,desc->DataLength,desc->DataType,desc->PoolId,desc->Pad[0]) ; 
                  TRACE(1,"_bad_ Chunk: OwnerHandle=%lx, BaseOrderId=%lu, ReferenceCount=%u, Pad=%u %u %u %u\n",
                                 chunk->OwnerHandle,chunk->BaseOrderId,chunk->ReferenceCount,chunk->Pad[0],chunk->Pad[1],chunk->Pad[2],chunk->Pad[3]);
                  for ( i=0 ; i<60 ; i++ )
                  {
                    ismStore_memReference_t *ref = &chunk->References[i] ; 
                    TRACE(1,"_bad_ Ref: i=%d, RefHandle=%lx, Value=%u, State=%u, Pad=%u %u %u\n",
                          i, ref->RefHandle, ref->Value, ref->State, ref->Pad[0],ref->Pad[1],ref->Pad[2]);
                  }
                  }
                 #endif
                } 
                if ( msg->DataLength < svsz )
                     msg->DataLength = svsz ;
              } 
            } 
          } 
          if ( hasProp[T2T[recType]] ) 
          {
            ismStore_GenId_t gid;
            if ( ism_store_getGenIdOfHandle((ismStore_Handle_t)sr->rec->Attribute, &gid) == ISMRC_OK && gid == genId )
            {
              svsz = prp->DataLength ; 
             #if USE_REC_TIME
               et = ism_common_readTSC() ; 
              rc = ism_store_memReadRecord((ismStore_Handle_t)sr->rec->Attribute, prp,1) ; 
               rt += ism_common_readTSC()-et ; 
             #else
              rc = ism_store_readRecord((ismStore_Handle_t)sr->rec->Attribute, prp,1) ; 
             #endif
              if ( rc == ISMRC_StoreBufferTooSmall )
              {
                void *tmp ; 
                if (!(tmp = ism_common_realloc(ISM_MEM_PROBE(ism_memory_store_misc,97),fiP->v[0], prp->DataLength)) )
                {
                  TRACE(1,"%s failed to allocate memory of %u bytes.\n",__FUNCTION__, prp->DataLength);
                  rc = ISMRC_AllocateError ;
                  goto exit ; 
                }
                fiP->l[0] = prp->DataLength ; 
                fiP->v[0] = tmp ; 
               #if USE_REC_TIME
                 et = ism_common_readTSC() ; 
                rc = ism_store_memReadRecord((ismStore_Handle_t)sr->rec->Attribute, prp,1) ; 
                 rt += ism_common_readTSC()-et ; 
               #else
                rc = ism_store_readRecord((ismStore_Handle_t)sr->rec->Attribute, prp,1) ; 
               #endif
              }
              if ( rc == ISMRC_OK && 
                   ((prp->Type >= ISM_STORE_RECTYPE_PROP && prp->Type <= ISM_STORE_RECTYPE_SPROP) || prp->Type == ISM_STORE_RECTYPE_RPROP) )
              {
                dt->dataType = ISM_STORE_DUMP_PROP ; 
                dt->handle = (ismStore_Handle_t)sr->rec->Attribute ; 
                dt->owner  = sr->handle ; 
                dt->pRecord = prp ; 
                if ( (rc = cbFunc(dt, cbCtx)) != ISMRC_OK ) goto exit ;
              }
              if ( prp->DataLength < svsz )
                   prp->DataLength = svsz ;
            } 
          }
        }
      }
    }
  }
  rc = ISMRC_OK ;
 exit:
  for ( recType=ISM_STORE_RECTYPE_SERVER ; recType<ISM_STORE_RECTYPE_MSG ; recType++ )
  {
    while ( first[recType] )
    {
      sr = first[recType] ;
      first[recType] = sr->next ; 
      if ( sr->fi->v[0] ) ism_common_free(ism_memory_store_misc,sr->fi->v[0]) ;
      ism_common_free(ism_memory_store_misc,sr) ; 
    }
  }
  if ( fiR->v[0] ) ism_common_free(ism_memory_store_misc,fiR->v[0]) ; 
  if ( fiM->v[0] ) ism_common_free(ism_memory_store_misc,fiM->v[0]) ; 
  if ( fiP->v[0] ) ism_common_free(ism_memory_store_misc,fiP->v[0]) ; 
 #if USE_REC_TIME
  dumpTime = ism_common_readTSC() -dumpStart - rt ; 
  TRACE(1,"%s time: %f (rt=%f)\n",__FUNCTION__,dumpTime,rt);
 #endif
  return rc ; 
}
#endif
/*-------------------------------*/
static int ism_store_countGenRecs(ismStore_GenId_t gid, uint32_t *rcnt, uint64_t *pStd)
{
  int rc ; 
  char *genData ;
  ismStore_memGenHeader_t *pGenHeader ;
  ismStore_memDescriptor_t *desc ; 
  uint64_t offset, blocksize, upto ; 
  double nn,aa,ss;

  if (!(genData = ism_store_getGen(gid,&rc)) )
    return rc ; 
  pGenHeader = (ismStore_memGenHeader_t *)genData ;

  nn = aa = ss = 0e0;
  if ( pGenHeader->CompactSizeBytes )
  {
    char *bptr, *eptr ; 
    size_t DS; 

    DS = pGenHeader->DescriptorStructSize ; 
    bptr = (char *)pGenHeader + pGenHeader->GranulePool[0].Offset ; 
    eptr = (char *)pGenHeader + pGenHeader->CompactSizeBytes ; 
    while ( bptr < eptr )
    {
      desc = (ismStore_memDescriptor_t *)bptr ; 
      if ( desc->DataType == ISM_STORE_RECTYPE_MSG )
      {
        double tl = (double)desc->TotalLength ; 
        nn++ ; 
        aa += tl ; 
        ss += tl*tl ; 
      }
      bptr += ALIGNED(DS+desc->DataLength) ; 
      rcnt[desc->DataType]++ ; 
    }
  }
  else
  {
    int i;
    offset = upto = i = blocksize = 0 ; 
    for(;;)
    {
      if ( offset >= upto )
      {
        if ( pGenHeader->PoolsCount > i && offset <= pGenHeader->GranulePool[i].Offset )
        {
          blocksize = pGenHeader->GranulePool[i].GranuleSizeBytes ; 
          offset    = pGenHeader->GranulePool[i].Offset ; 
          upto      = offset + pGenHeader->GranulePool[i].MaxMemSizeBytes ; 
          i++ ; 
        }
        else
          break ; 
      }
      desc = (ismStore_memDescriptor_t *)((uintptr_t)pGenHeader + offset) ;
      if ( desc->DataType == ISM_STORE_RECTYPE_MSG )
      {
        double tl = (double)desc->TotalLength ; 
        nn++ ; 
        aa += tl ; 
        ss += tl*tl ; 
      }
      rcnt[desc->DataType]++ ; 
      offset += blocksize ; 
    }
  }
  if ( nn > 1 )
  {
    aa /= nn ; 
    *pStd = (uint64_t)sqrt((ss - nn*aa*aa)/(nn-1)) ; 
  }
  else
    *pStd = 0 ; 
  return ISMRC_OK ; 
}
/*-------------------------------*/
static char *type2str(int type, char *str, int len)
{
  int k=0 ; 
  if ( type & ismSTORE_DATATYPE_NOT_PRIMARY )
  {
    k += su_strlcpy(str+k,"Ext_of_",len>=k?len-k:0);
    type &= ~ismSTORE_DATATYPE_NOT_PRIMARY ; 
  }
  if ( type >= ISM_STORE_RECTYPE_SERVER &&
       type < ISM_STORE_RECTYPE_MAXVAL )
    k += su_strlcpy(str+k,recName(type),len>=k?len-k:0);
  else if ( type == ismSTORE_DATATYPE_FREE_GRANULE )
    k += su_strlcpy(str+k,"FREE_GRANULE",len>=k?len-k:0);
  else if ( type == ismSTORE_DATATYPE_NEWLY_HATCHED )
    k += su_strlcpy(str+k,"NEWLY_HATCHED",len>=k?len-k:0);
  else if ( type == ismSTORE_DATATYPE_MGMT )
    k += su_strlcpy(str+k,"MGMT",len>=k?len-k:0);
  else if ( type == ismSTORE_DATATYPE_GENERATION )
    k += su_strlcpy(str+k,"GENERATION",len>=k?len-k:0);
  else if ( type == ismSTORE_DATATYPE_GEN_IDS )
    k += su_strlcpy(str+k,"GEN_IDS",len>=k?len-k:0);
  else if ( type == ismSTORE_DATATYPE_STORE_TRAN )
    k += su_strlcpy(str+k,"STORE_TRAN",len>=k?len-k:0);
  else if ( type == ismSTORE_DATATYPE_LDATA_EXT )
    k += su_strlcpy(str+k,"LDATA_EXT",len>=k?len-k:0);
  else if ( type == ismSTORE_DATATYPE_REFERENCES )
    k += su_strlcpy(str+k,"REFERENCES",len>=k?len-k:0);
  else if ( type == ismSTORE_DATATYPE_REFSTATES )
    k += su_strlcpy(str+k,"REFSTATES",len>=k?len-k:0);
  else if ( type == ismSTORE_DATATYPE_STATES )
    k += su_strlcpy(str+k,"STATES",len>=k?len-k:0);
  else
    k += snprintf(str+k,len>=k?len-k:0,"Unknown type (0x%4.4x)",type);
  return str ; 
}
/*-------------------------------*/
XAPI int32_t ism_store_dumpRaw(char *path)
{
  char stp[128];
  FILE *fp ;
  int32_t rc = ISMRC_OK, need_close=0 ;
  size_t rlen;
  uint32_t *rcnt, *scnt, ng=0;
  ismStore_memMgmtHeader_t *pMgmHeader;
  ismStore_Handle_t handle ; 
  ismStore_memDescriptor_t *desc ;
  ismStore_memGenIdChunk_t *chunk ;

  if (ismStore_memGlobal.State != ismSTORE_STATE_RECOVERY && ismStore_memGlobal.State != ismSTORE_STATE_ACTIVE)
  {
    TRACE(1, "Failed to dump the store's content, because the store's state (%d) is not valid\n", ismStore_memGlobal.State);
    return ISMRC_StoreNotAvailable;
  }

  rlen = (1<<16)*sizeof(int) ; 
  if ( !(rcnt = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,103),2*rlen)) )
    return ISMRC_AllocateError ;
  scnt = rcnt + (1<<16) ;
  memset(scnt,0,rlen) ; 

  if (!path || !strcmp(path,"stdout") )
    fp = stdout ; 
  else
  if (!strcmp(path,"stderr") )
    fp = stderr ; 
  else
  if ( !(fp = fopen(path,"we")) ) 
    fp = stdout ; 
  else
    need_close = 1 ; 

  pMgmHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress;
  recTimes[3] = su_sysTime() ; 
  for ( handle=pMgmHeader->GenIdHandle ; handle ; handle=desc->NextHandle ) 
  {
    int i;
    desc = (ismStore_memDescriptor_t *)(ismStore_memGlobal.pStoreBaseAddress + ismSTORE_EXTRACT_OFFSET(handle)) ;
    chunk = (ismStore_memGenIdChunk_t *)((uintptr_t)desc+pMgmHeader->DescriptorStructSize) ;
    for ( i=0 ; i<chunk->GenIdCount ; i++ )
    {
      uint64_t std=0 ; 
      ng++;
      memset(rcnt,0,rlen) ; 
      if ( (rc = ism_store_countGenRecs(chunk->GenIds[i], rcnt, &std)) == ISMRC_OK )
      {
        int j;
        fprintf(fp,"Content of Generation %u:",chunk->GenIds[i]);
        for ( j=0 ; j<65536 ; j++ ) if ( rcnt[j] ) {scnt[j]+=rcnt[j] ; fprintf(fp," %u records of type %s ,",rcnt[j],type2str(j,stp,sizeof(stp))) ;} 
        if ( std )
          fprintf(fp," StdDev of Msg Recs: %lu ",std);
        fprintf(fp,"\n");
      }
      else
        fprintf(fp,"Failed to count records of generation %u, rc= %d\n",chunk->GenIds[i],rc);
    }
  }
  {
    int j;
    fprintf(fp,"Total Store Content (ngens %u):",ng);
    for ( j=0 ; j<65536 ; j++ ) if ( scnt[j] ) fprintf(fp," %u records of type %s ,",scnt[j],type2str(j,stp,sizeof(stp))) ; 
    fprintf(fp,"\n");
  }
  recTimes[4] = su_sysTime() ; 
  ism_common_free(ism_memory_store_misc,rcnt);
  if ( need_close )
    fclose(fp) ; 

  return ISMRC_OK ; 
}
/*-------------------------------*/

static char *print_record(ismStore_Record_t *pR, char *str, int len)
{
  int i,k=0, j=0 ; 
  char *p;
  memset(str,0,len);
  k += snprintf(str+k,len-k,"Type=%x, Attr=%lx, State=%lx, dLen=%u, ",
       pR->Type, pR->Attribute, pR->State, pR->DataLength);
  for ( i=0, p=pR->pFrags[0] ; i<pR->DataLength && k<len ; i++, p++ )
  {
    if ( isprint(*p) )
    {
      if ( j )
      {
        str[k++] = '|' ; 
        j = 0 ; 
      }
      str[k++] = *p ; 
    }
    else
    {
      unsigned char c;
      if ( !j )
      {
        str[k++] = '|' ; 
        j = 1 ; 
      }
      c = (*p)>>4 ;  
      if ( c < 10 )
        str[k++] = '0' + c ; 
      else
        str[k++] = 'a' + c-10 ; 
      c = (*p)&0xf ; 
      if ( c < 10 )
        str[k++] = '0' + c ; 
      else
        str[k++] = 'a' + c-10 ; 
    }
  }
  return str ; 
}
/*-------------------------------*/
static int dump2file(ismStore_DumpData_t *pDumpData, void *pContext)
{
  char str[4096] ; 
  ismStore_DumpData_t *dt = pDumpData ; 
  FILE *fp = (FILE *)pContext ; 
  if (!(dt && fp) ) return ISMRC_NullArgument ;

  switch (dt->dataType)
  {
   case ISM_STORE_DUMP_GENID :
    fprintf(fp," Data for generation %hu\n",dt->genId);
    break ; 
   case ISM_STORE_DUMP_REC4TYPE :
   {
    ismStore_ReferenceStatistics_t rs[1] ; 
    if ( ismSTORE_IS_SPLITITEM(dt->recType) && ism_store_getReferenceStatistics(dt->handle,rs) == ISMRC_OK )
      fprintf(fp,"\tHandle %p: %s , min_act_oid %lu, max_oid %lu ; definition: %s\n",(void *)dt->handle, recName(dt->recType),
              rs->MinimumActiveOrderId,rs->HighestOrderId,print_record(dt->pRecord,str,sizeof(str)));
    else
      fprintf(fp,"\tHandle %p: %s definition: %s\n",(void *)dt->handle, recName(dt->recType),print_record(dt->pRecord,str,sizeof(str)));
    break ; 
   }
   case ISM_STORE_DUMP_REF4OWNER :
    fprintf(fp,"\t\tReference for %s record at handle %p in generation %hu: ",recName(dt->recType),(void *)dt->owner, dt->genId); 
    fprintf(fp," Reference %p, referring to %p, orderId %lu, value %u, state %d.\n", (void *)dt->handle, (void *)dt->pReference->hRefHandle, dt->pReference->OrderId, dt->pReference->Value, (int)dt->pReference->State);
    dt->readRefHandle = 1 ; 
    break ; 
   case ISM_STORE_DUMP_REF :
    dt->readRefHandle = 1 ; 
    break ; 
   case ISM_STORE_DUMP_MSG :
    fprintf(fp,"\t\t\t\tMessage - length %u, attr %lu, state %lu.\n", dt->pRecord->DataLength,dt->pRecord->Attribute,dt->pRecord->State); 
    break ; 
   case ISM_STORE_DUMP_PROP :
    fprintf(fp,"\t\tProperty at handle %p, for %s owner %p in generation %hu: %s\n",(void *)dt->handle,recName(dt->recType),(void *)dt->owner, dt->genId,print_record(dt->pRecord,str,sizeof(str))); 
    break ; 
   default :
    fprintf(fp,"Unrecognized DUMP dataType: %d.\n",dt->dataType) ; 
    break ; 
  }
  return ISMRC_OK ;
}
/*-------------------------------*/
XAPI int32_t ism_store_memDump(char *path)
{
  FILE *fp ;
  int32_t rc = ISMRC_OK, need_close=0 ; 

  if (!path || !strcmp(path,"stdout") )
    fp = stdout ; 
  else
  if (!strcmp(path,"stderr") )
    fp = stderr ; 
  else
  if ( !(fp = fopen(path,"we")) ) 
    fp = stdout ; 
  else
    need_close = 1 ; 

  rc = ism_store_memDumpCB(dump2file, fp) ;
  if ( need_close ) fclose(fp) ; 

  return rc ;
}
/*-------------------------------*/
XAPI int32_t ism_store_memRecoveryCompletionPct(void)
{
  int ng = numGens;
  if ( isOn != 2 ) return -1 ; 
  ng = ng>0 ? 100*curGens/ng : 0; 
  if ( ng < 1 ) ng = 1 ; 
  else
  if ( ng > 99) ng = 99; 
  return ng ; 
}
/*-------------------------------*/
static inline int getGidInd(ismStore_Handle_t handle)
{
  int gid ; 
  ismStore_memGenInfo_t *gi ; 

  gid = ismSTORE_EXTRACT_GENID(handle) ; 
  if ( gid < minGen || gid > maxGen )
    return -1 ; 
  if ( gid == ismSTORE_MGMT_GEN_ID )
  {
    ismStore_memGenHeader_t  *pGenHeader;
    ismStore_memDescriptor_t *desc ;
    ismStore_memSplitItem_t *si ;
    char *genData;
  
    genData = ismStore_memGlobal.pStoreBaseAddress ;
    pGenHeader = (ismStore_memGenHeader_t *)genData ;
    desc = (ismStore_memDescriptor_t *)(genData + ismSTORE_EXTRACT_OFFSET(handle)) ;
    if (!ismSTORE_IS_SPLITITEM(desc->DataType) )
      return -1 ; 
    si = (ismStore_memSplitItem_t *)((uintptr_t)desc + pGenHeader->DescriptorStructSize) ; 
    gid = ismSTORE_EXTRACT_OFFSET(si->hLargeData) ? ismSTORE_EXTRACT_GENID(si->hLargeData) : gid1st ;
  }
  gi = allGens + (gid-minGen) ;
  return gi->genInd ; 
}
XAPI int32_t ism_store_compareHandles(ismStore_Handle_t handle1, ismStore_Handle_t handle2, int *pResult)
{
  int i1,i2 ; 
  if (!pResult )
  {
    ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "pResult");
    return ISMRC_ArgNotValid ; 
  }
  if ( isOn != 2 )
    return ISMRC_StoreNotAvailable ; 
  if ( (i1 = getGidInd(handle1)) < 0 )
  {
    ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "handle1");
    return ISMRC_ArgNotValid ; 
  }
  if ( (i2 = getGidInd(handle2)) < 0 )
  {
    ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "handle2");
    return ISMRC_ArgNotValid ; 
  }
  *pResult = (i1-i2) ; 
  return ISMRC_OK ; 
}
