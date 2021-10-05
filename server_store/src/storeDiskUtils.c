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
/* Module Name: DiskUtils.c                                          */
/*                                                                   */
/* Description: Disk utilities file                                  */
/*                                                                   */
/*********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#define TRACE_COMP Store
#define _ATFILE_SOURCE
#define _FILE_OFFSET_BITS 64

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
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
#include "storeRecovery.h"
#include "storeUtils.h"
#include "storeMemory.h"

#define DU_MAX_PRIO  3

typedef enum
{
  DUJOB_STORE_TERM,
  DUJOB_STORE_WRITE,
  DUJOB_STORE_READ,
  DUJOB_STORE_DELETE,
  DUJOB_STORE_LIST,
  DUJOB_STORE_COMP,
  DUJOB_STORE_WIPE
} ismStore_diskUtilsJobType;

typedef struct
{
  ismStore_DiskBufferParams_t  BufferParams[1] ; 
  ismPSTOREDISKASYNCCALLBACK   callback ;
  void                        *pContext ; 
  ismStoe_DirInfo             *di ; 
  ismStore_GenId_t             GenId ; 
} ismStore_diskUtilsStoreJobInfo ;

typedef struct ismStore_diskUtilsJob
{
  struct ismStore_diskUtilsJob  *next_job ; 
  void                          *job_info ; 
  ismStore_diskUtilsJobType      job_type ; 
  int                            job_prio ; 
  int                            job_term ; 
  int                            job_dead ; 
  int                            job_live ; 
  int                            job_errno ; 
  int                            job_line ; 
} ismStore_diskUtilsJob ; 

typedef struct ismStoe_DiskUtilsCtx
{
  ism_threadh_t            tid;
  pthread_mutex_t          lock;
  pthread_cond_t           cond;
  ismStore_diskUtilsJob   *head[DU_MAX_PRIO];
  ismStore_diskUtilsJob   *tail[DU_MAX_PRIO];
  int                      goOn ; 
  int                      thUp ; 
  volatile int             stop ; 
} ismStoe_DiskUtilsCtx ; 

/********************************************************/
extern ismStore_memGlobal_t ismStore_memGlobal;
/********************************************************/
static size_t TransferBlockSize ; 
static uint64_t mask[64];
static ismStoe_DirInfo genDir[1] ; 
static ismStoe_DiskUtilsCtx *pCtx=NULL;
static pthread_mutex_t gLock = PTHREAD_MUTEX_INITIALIZER ; 
static void *ism_store_diskUtilsThread(void *arg, void * context, int value);
static int ism_storeDisk_ioFile(char *fn, int ioIn, ismStore_diskUtilsJob *job) ;

#define ism_store_isGenName(x) (x[0]=='g' && isdigit(x[1]) && isdigit(x[2]) && isdigit(x[3]) && isdigit(x[4]) && isdigit(x[5]) && isdigit(x[6]) && !x[7])
int ism_store_isTmpName(const char *x)
{
  if ( memcmp(x,"tid_",4) )
    return 0 ; 
  x += 4 ; 
  for ( ; *x ; x++ )
    if (!isdigit(*x) )
      return 0 ; 
  return 1 ; 
}
/********************************************************/

/*------------------------------------------------------*/
int ism_storeDisk_initDir(const char *path, ismStoe_DirInfo *di)
{
  int rc=StoreRC_OK ; 
  int iok=0, fdir ; 
  size_t plen, block ; 
  DIR   *pdir=NULL ; 
  struct statfs sfs[1] ; 

  if ( !(path && di) )
    return StoreRC_BadParameter ; 
  memset(di, 0, sizeof(ismStoe_DirInfo)) ; 

  do
  {
    if ( access(path, F_OK|R_OK|W_OK) )
    {
      int ec = errno ; 
      rc = StoreRC_BadParameter ; 
      TRACE(1, "%s: system call 'access' failed. errno %d (%s), path %s, mode F_OK|R_OK|W_OK .\n",__FUNCTION__, ec, strerror(ec), path);
      if ( ec == EACCES || ec == ENOENT )
      {
        if ( access(path, F_OK) )
        {
          ec = errno ; 
          if ( ec == EACCES || ec == ENOENT )
          {
            if ( mkdir(path, ismStore_memGlobal.PersistedDirectoryMode) )
            {
              ec = errno ; 
              TRACE(1, "%s: the directory %s does not exist and an attempt to create it failed with errno %d (%s).\n",__FUNCTION__, path,ec,strerror(ec));
              rc = StoreRC_BadParameter ; 
            }
            else
            {
              TRACE(1, "%s: the directory %s does not exist and successfully created.\n",__FUNCTION__, path);
              rc = StoreRC_OK ;
            }
          }
        }
      }
      if ( rc != StoreRC_OK )
        break ; 
    }
    plen = su_strllen(path, PATH_MAX+1) ; 
    if ( plen < 1 || plen > PATH_MAX )
    {
      rc = StoreRC_BadParameter ; 
      break ; 
    }
    if (!(pdir = opendir(path)) )
    {
      rc = StoreRC_BadParameter ; 
      break ; 
    }
    iok++ ; 
    if ( !(di->path = su_strdup(path)) )
    {
      rc = StoreRC_AllocateError ;
      break ; 
    }
    iok++ ; 
    fdir = dirfd(pdir) ; 
    if ( fstatfs(fdir, sfs) )
      block = getpagesize() ; 
    else
      block = sfs->f_bsize ;  
    di->plen = plen ; 
    di->pdir = pdir ; 
    di->fdir = fdir ; 
    di->block = block ; 
    di->batch = TransferBlockSize > (4*block) ? TransferBlockSize : (4*block) ; 
    di->batch = (di->batch+di->block-1)/di->block*di->block ; 
    iok++ ; 
  } while(0) ; 
  if ( iok < 3 )
  {
    if ( iok > 1 )
      ism_common_free(ism_memory_store_misc,di->path);
    if ( iok > 0 )
      closedir(pdir) ; 
  }
  return rc ; 
}
/*------------------------------------------------------*/
static ismStoe_DirInfo *ism_storeDisk_getDirInfo(const char *path, const char *file)
{
  int iok=0;
  ismStoe_DirInfo *di ; 

  if ( !(path && file) )
    return NULL ; 

  do
  {
    if ( !(di = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,214),sizeof(ismStoe_DirInfo))) )
      break ; 
    iok++ ; 
    if ( ism_storeDisk_initDir(path, di) != StoreRC_OK )
      break ; 
    iok++ ; 
    if ( !(di->file = ism_common_strdup(ISM_MEM_PROBE(ism_memory_store_misc,1000),file)) )
      break ; 
    iok++ ; 
  } while (0) ; 
  if ( iok < 3 )
  {
    if ( iok > 1 )
      ism_common_free(ism_memory_store_misc,di->path) ; 
    if ( iok > 0 )
      ism_common_free(ism_memory_store_misc,di) ; 
    di = NULL ; 
  }
  return di ; 
}
/*------------------------------------------------------*/
static void ism_storeDisk_delDirInfo(ismStoe_DirInfo *di)
{

  if (  di && di != genDir )
  {
    if ( di->file ) ism_common_free(ism_memory_store_misc,di->file);
    if ( di->path ) ism_common_free(ism_memory_store_misc,di->path);
    closedir(di->pdir) ; 
    ism_common_free(ism_memory_store_misc,di) ; 
  }
  return ; 
}
/*------------------------------------------------------*/
static inline void ism_storeDisk_compactRefChunk(ismStore_memDescriptor_t *desc, size_t DS)
{
  size_t j,k,l;
  ismStore_memReferenceChunk_t *rfchk = (ismStore_memReferenceChunk_t *)((uintptr_t)desc+DS) ;
  for ( j=0 ; j<rfchk->ReferenceCount && !rfchk->References[j].RefHandle ; j++ ) ; // empty body
  for ( k=rfchk->ReferenceCount-1 ; k>j && !rfchk->References[k].RefHandle ; k-- ) ; // empty body
  if ( k<j )
  {
    l = 1 ; 
    desc->DataLength -= (rfchk->ReferenceCount - l) * sizeof(ismStore_memReference_t) ; 
    rfchk->ReferenceCount = l ; 
  }
  else
  if ( (l = k+1-j) < rfchk->ReferenceCount )
  {
    if ( j>0 )
    {
      if ( l <= j )
        memcpy( rfchk->References, rfchk->References+j, l*sizeof(ismStore_memReference_t)) ; 
      else
        memmove(rfchk->References, rfchk->References+j, l*sizeof(ismStore_memReference_t)) ; 
    }
  //memset(rfchk->References+l, 0, j*sizeof(ismStore_memReference_t)) ; 
    rfchk->BaseOrderId += j ; 
    desc->DataLength -= (rfchk->ReferenceCount - l) * sizeof(ismStore_memReference_t) ; 
    rfchk->ReferenceCount = l ; 
  }
}
/*------------------------------------------------------*/
static int ism_storeDisk_deflateMemGen(void *genData, uint64_t **bitMaps, ismStore_DiskBufferParams_t *bp)
{
  int i, rc , pos, bit, DS, crc, pc;
  uint64_t offset, idx, upto, blocksize , newSize, poolOff, *bitMap=NULL ; 
  double nn,aa,ss, t1,t2,t3,t4;
  ismStore_memDescriptor_t *desc ;
  ismStore_Handle_t handle ; 
  ismStore_memGenHeader_t *pGenHeader = (ismStore_memGenHeader_t *)genData ; 
  void *buff, *bptr ; 

  DS = pGenHeader->DescriptorStructSize ; 
  newSize = pGenHeader->MemSizeBytes ; 
  i = getpagesize() ; 
  t1 = su_sysTime() ; 
  if ( (rc=posix_memalign(&buff, i, newSize)) ) 
  {
    rc = StoreRC_AllocateError ;
    return rc ; 
  }
  t2 = su_sysTime() ; 
  bptr = buff ; 
//TRACE(1,"Compacting: gid=%d, sizes: %lu, %lu\n",pGenHeader->GenId, pGenHeader->MemSizeBytes,newSize);
  memcpy(bptr, pGenHeader, pGenHeader->GranulePool[0].Offset) ; bptr += pGenHeader->GranulePool[0].Offset ; 
  nn = aa = ss = 0e0;
  pc = pGenHeader->PoolsCount <= ismSTORE_GRANULE_POOLS_COUNT ? pGenHeader->PoolsCount : ismSTORE_GRANULE_POOLS_COUNT ; 
  if ( pGenHeader->GenId == ismSTORE_MGMT_GEN_ID )
  {
    uint16_t DataType;
    i = 0 ; 
    if ( i<pc )
    {
      blocksize = pGenHeader->GranulePool[i].GranuleSizeBytes ; 
      poolOff   = pGenHeader->GranulePool[i].Offset ; 
      upto      = pGenHeader->GranulePool[i].MaxMemSizeBytes + poolOff; 
      for ( offset=poolOff ; offset<upto ; offset += blocksize )
      {
        desc = (ismStore_memDescriptor_t *)((uintptr_t)pGenHeader + offset) ;
        DataType = desc->DataType & (~ismSTORE_DATATYPE_NOT_PRIMARY) ; 
        if ( DataType != ismSTORE_DATATYPE_FREE_GRANULE &&
             DataType != ismSTORE_DATATYPE_NEWLY_HATCHED &&
             DataType != 0 )
        {
          desc->GranuleIndex = (offset - poolOff) / blocksize ; 
          memcpy(bptr, desc, ALIGNED(DS+desc->DataLength)) ; 
          bptr += ALIGNED(DS+desc->DataLength) ; 
        }
        else
        {
          ismStore_memSplitItem_t *si = (ismStore_memSplitItem_t *)(desc+1) ; 
          if ( si->Version )
          {
            uint32_t len = offsetof(ismStore_memSplitItem_t,Version) + sizeof(si->Version) ; 
            desc->GranuleIndex = (offset - poolOff) / blocksize ; 
            memcpy(bptr, desc, ALIGNED(DS+len)) ; 
            desc = (ismStore_memDescriptor_t *)bptr ; 
            desc->DataLength = len ; 
            desc->DataType = 0;
            bptr += ALIGNED(DS+len) ; 
          }
        }
      }
    }
    i++ ; 
    if ( i<pc )
    {
      blocksize = pGenHeader->GranulePool[i].GranuleSizeBytes ; 
      poolOff   = pGenHeader->GranulePool[i].Offset ; 
      upto      = pGenHeader->GranulePool[i].MaxMemSizeBytes + poolOff; 
      for ( offset=poolOff ; offset<upto ; offset += blocksize )
      {
        desc = (ismStore_memDescriptor_t *)((uintptr_t)pGenHeader + offset) ;
        DataType = desc->DataType & (~ismSTORE_DATATYPE_NOT_PRIMARY) ; 
        if ( DataType != ismSTORE_DATATYPE_FREE_GRANULE &&
             DataType != ismSTORE_DATATYPE_NEWLY_HATCHED &&
             DataType != ismSTORE_DATATYPE_STORE_TRAN &&
             DataType != 0 )
        {
          desc->GranuleIndex = (offset - poolOff) / blocksize ; 
          memcpy(bptr, desc, ALIGNED(DS+desc->DataLength)) ; 
          bptr += ALIGNED(DS+desc->DataLength) ; 
        }
      }
    }
    i++ ; 
    for(     ; i<pc ; i++ )
    {
      blocksize = pGenHeader->GranulePool[i].GranuleSizeBytes ; 
      poolOff   = pGenHeader->GranulePool[i].Offset ; 
      upto      = pGenHeader->GranulePool[i].MaxMemSizeBytes + poolOff; 
      for ( offset=poolOff ; offset<upto ; offset += blocksize )
      {
        desc = (ismStore_memDescriptor_t *)((uintptr_t)pGenHeader + offset) ;
        DataType = desc->DataType & (~ismSTORE_DATATYPE_NOT_PRIMARY) ; 
        if ( DataType != ismSTORE_DATATYPE_FREE_GRANULE &&
             DataType != ismSTORE_DATATYPE_NEWLY_HATCHED &&
             DataType != ismSTORE_DATATYPE_STORE_TRAN &&
             DataType != 0 )
        {
          desc->GranuleIndex = (offset - poolOff) / blocksize ; 
          memcpy(bptr, desc, ALIGNED(DS+desc->DataLength)) ; 
          bptr += ALIGNED(DS+desc->DataLength) ; 
        }
      }
    }
  }
  else
  if ( bitMaps )
  {
    for( i=0 ; i<pc ; i++ )
    {
      blocksize = pGenHeader->GranulePool[i].GranuleSizeBytes ; 
      poolOff   = pGenHeader->GranulePool[i].Offset ; 
      upto      = pGenHeader->GranulePool[i].MaxMemSizeBytes / blocksize ; 
      bitMap    = bitMaps[i] ; 
      for ( idx=0 ; idx<upto ; idx++ )
      {
        pos = (idx>>6) ; 
        bit = (idx&0x3f) ; 
        if ( bitMap[pos]&mask[bit] )
        {
          double tl=0e0;
          offset = poolOff + (idx * blocksize) ; 
          desc = (ismStore_memDescriptor_t *)((uintptr_t)pGenHeader + offset) ;
          crc = (bp->fCompactRefChunks && desc->DataType == ismSTORE_DATATYPE_REFERENCES ) ; 
          for ( handle=offset ; handle ; handle=ismSTORE_EXTRACT_OFFSET(desc->NextHandle) )
          {
            desc = (ismStore_memDescriptor_t *)((uintptr_t)pGenHeader + handle) ;
            desc->GranuleIndex = (handle - poolOff) / blocksize ; 
            memcpy(bptr, desc, ALIGNED(DS+desc->DataLength)) ; 
            desc = (ismStore_memDescriptor_t *)bptr ; 
            if ( crc ) ism_storeDisk_compactRefChunk(desc, DS) ; 
            bptr += ALIGNED(DS+desc->DataLength) ;
            tl += (double)ALIGNED(DS+desc->DataLength) ; 
          }
          nn++ ; 
          aa += tl ; 
          ss += tl*tl ; 
        }
      }
    }
  }
  else
  {
    for( i=0 ; i<pc ; i++ )
    {
      blocksize = pGenHeader->GranulePool[i].GranuleSizeBytes ; 
      poolOff   = pGenHeader->GranulePool[i].Offset ; 
      upto      = pGenHeader->GranulePool[i].MaxMemSizeBytes + poolOff; 
      for ( offset=poolOff ; offset<upto ; offset += blocksize )
      {
        desc = (ismStore_memDescriptor_t *)((uintptr_t)pGenHeader + offset) ;
        if ( ismSTORE_IS_HEADITEM(desc->DataType) )
        {
          double tl=0e0;
          crc = (bp->fCompactRefChunks && desc->DataType == ismSTORE_DATATYPE_REFERENCES ) ; 
          for ( handle=offset ; handle ; handle=ismSTORE_EXTRACT_OFFSET(desc->NextHandle) )
          {
            desc = (ismStore_memDescriptor_t *)((uintptr_t)pGenHeader + handle) ;
            desc->GranuleIndex = (handle - poolOff) / blocksize ; 
            memcpy(bptr, desc, ALIGNED(DS+desc->DataLength)) ; 
            desc = (ismStore_memDescriptor_t *)bptr ; 
            if ( crc ) ism_storeDisk_compactRefChunk(desc, DS) ; 
            bptr += ALIGNED(DS+desc->DataLength) ; 
            tl += (double)ALIGNED(DS+desc->DataLength) ; 
          }
          nn++ ; 
          aa += tl ; 
          ss += tl*tl ; 
        }
      }
    }
  }
  t3 = su_sysTime() ; 
  bp->pBuffer = buff ; 
  newSize = bptr - buff ; 
  pGenHeader = (ismStore_memGenHeader_t *)bp->pBuffer ; 
  pGenHeader->CompactSizeBytes = newSize ; 
  newSize = (newSize + (genDir->block-1)) & (~(genDir->block-1)) ; 
  bp->BufferLength = newSize ;
  if ( nn > 1 )
  {
    aa /= nn ; 
    bp->StdDev = (uint64_t)sqrt((ss - nn*aa*aa)/(nn-1)) ; 
  }
  else
    bp->StdDev = 0 ; 
  pGenHeader->StdDevBytes = bp->StdDev ; 
  if ( (buff = ism_common_realloc_memaligned(ISM_MEM_PROBE(ism_memory_store_misc,220),buff,newSize)) )
    bp->pBuffer = buff ; 
  t4 = su_sysTime() ; 
  pGenHeader = (ismStore_memGenHeader_t *)bp->pBuffer ; // in case it was changed by realloc
  TRACE(5,"Compacting: gid=%d, sizes: %lu, %lu, times: alloc=%f, compact=%f, realloc=%f\n",pGenHeader->GenId, pGenHeader->MemSizeBytes,pGenHeader->CompactSizeBytes,t2-t1,t3-t2,t4-t3);
  return StoreRC_OK ; 
}
/*------------------------------------------------------*/
static int ism_storeDisk_reflateMemGen(uint64_t **bitMaps, ismStore_DiskBufferParams_t *bp)
{
  int pos, bit, DS ; 
  uint64_t movSize, *bitMap, newSize ; 
  double nn,aa,ss;
  ismStore_memDescriptor_t *desc ;
  ismStore_memGenHeader_t *pGenHeader = (ismStore_memGenHeader_t *)bp->pBuffer ; 
  char *bptr, *eptr, *pptr, *tptr ; 

  DS = pGenHeader->DescriptorStructSize ; 
  bptr = bp->pBuffer + pGenHeader->GranulePool[0].Offset ; 
  eptr = bp->pBuffer + pGenHeader->CompactSizeBytes ; 
  pptr = bptr ; 
  nn = aa = ss = 0e0;
  while ( bptr < eptr )
  {
    desc = (ismStore_memDescriptor_t *)bptr ; 
    bitMap = bitMaps[desc->PoolId] ; 
    pos    = (desc->GranuleIndex>>6) ; 
    bit    = (desc->GranuleIndex&0x3f) ; 
    if ( bitMap[pos]&mask[bit] )
    {
      double tl;
      tptr = bptr ; 
      while ( bptr < eptr )
      {
        bptr += ALIGNED(DS+desc->DataLength) ;
        if ( ismSTORE_EXTRACT_OFFSET(desc->NextHandle) )
          desc = (ismStore_memDescriptor_t *)bptr ; 
        else
          break ; 
      }
      movSize = bptr - tptr ; 
      tl = (double)movSize ; 
      nn++ ; 
      aa += tl ; 
      ss += tl*tl ; 
      if ( pptr < tptr )
      {
        if ( pptr + movSize > tptr )
          memmove(pptr,tptr,movSize) ; 
        else
          memcpy( pptr,tptr,movSize) ; 
      }
      pptr += movSize ; 
    }
    else
    {
      bptr += ALIGNED(DS+desc->DataLength) ;
    }
  }

  newSize = pptr - bp->pBuffer ; 
  pGenHeader->CompactSizeBytes = newSize ; 
  newSize = (newSize + (genDir->block-1)) & (~(genDir->block-1)) ; 
  bp->BufferLength = newSize ; 
  if ( nn > 1 )
  {
    aa /= nn ; 
    bp->StdDev = (uint64_t)sqrt((ss - nn*aa*aa)/(nn-1)) ; 
  }
  else
    bp->StdDev = 0 ; 
  pGenHeader->StdDevBytes = bp->StdDev ; 
  if ( (bptr = ism_common_realloc_memaligned(ISM_MEM_PROBE(ism_memory_store_misc,221),bp->pBuffer,newSize)) )
    bp->pBuffer = bptr ;   
  return StoreRC_OK ; 
}
/*------------------------------------------------------*/
#if 0
#include <execinfo.h>
void print_stack_trace(void)
{
  void *bt[256] ; 
  char **lines ; 
  int i,n;
  n = backtrace(bt,256);
  if ( (lines = backtrace_symbols(bt,n)) ) 
  {
    for ( i=0 ; i<n ; i++ )
      TRACE(1,"_DBG_backtrace: %s\n",lines[i]);
    ism_common_free(ism_memory_store_misc,lines) ; 
  }
}
#endif
/*------------------------------------------------------*/
int ism_storeDisk_init(ismStore_DiskParameters_t *pStoreDiskParams)
{
  int rc=StoreRC_OK ; 
  int i,iok=0, oki ; 

  pthread_mutex_lock(&gLock) ; 
  do
  {
    if ( pCtx )
    {
      TRACE(1, "%s: already on.\n", __FUNCTION__);
      rc = StoreRC_Disk_AlreadyOn ;
      break ; 
    }
    if ( !pStoreDiskParams )
    {
      TRACE(1, "%s: the argument 'pStoreDiskParams' is NULL.\n", __FUNCTION__);
      rc = StoreRC_BadParameter ; 
      break ; 
    }
    for ( i=0 ; i<64 ; i++ )
    {
      mask[i] = 1 ; 
      mask[i] <<= i ; 
    }
    TransferBlockSize = pStoreDiskParams->TransferBlockSize ;
    rc = ism_storeDisk_initDir(pStoreDiskParams->RootPath, genDir) ; 
    if ( rc != StoreRC_OK )
      break ; 
    iok++ ; 
    pStoreDiskParams->DiskBlockSize = genDir->block ; 
    genDir->batch = pStoreDiskParams->TransferBlockSize ;
    if ( genDir->batch < genDir->block )
         genDir->batch = genDir->block ;
    genDir->batch = (genDir->batch+genDir->block-1)/genDir->block*genDir->block ; 
    {
      struct dirent *de ; 
      rewinddir(genDir->pdir);
      while ( (de = readdir(genDir->pdir)) )
      {
       #ifdef _DIRENT_HAVE_D_TYPE
        if ( de->d_type && de->d_type != DT_REG ) continue ; 
       #endif
        if ( ism_store_isTmpName(de->d_name) ||
            (pStoreDiskParams->ClearStoredFiles && ism_store_isGenName(de->d_name)) )
        {
          if ( unlinkat(genDir->fdir, de->d_name,0) )
          {
            rc = StoreRC_SystemError ; 
            break ; 
          }
        }
      }
    } 
    if ( !(pCtx = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,223),sizeof(ismStoe_DiskUtilsCtx))) )
    {
      rc = StoreRC_AllocateError ;
      break ; 
    }
    iok++ ; 
    memset(pCtx,0,sizeof(ismStoe_DiskUtilsCtx)) ; 
    do
    {
      char *th_nm;
      oki = 0  ; 
      if ( pthread_mutex_init(&pCtx->lock,NULL) )
      {
        rc = StoreRC_SystemError ; 
        break ; 
      }
      oki++ ; 
      if ( pthread_cond_init(&pCtx->cond,NULL) )
      {
        rc = StoreRC_SystemError ; 
        break ; 
      }
      oki++ ; 
      pCtx->goOn = 2 ; 
      th_nm = "diskUtil" ; 
      if ( ism_common_startThread(&pCtx->tid, ism_store_diskUtilsThread, pCtx, NULL, 0, ISM_TUSAGE_NORMAL,0, th_nm, "Read_and_Write_Generations_from_to_Disk") )
      {
        rc = StoreRC_SystemError ; 
        break ; 
      }
      oki++ ; 
    } while(0) ; 
    if ( oki < 3 )
    {
      if ( oki > 1 )
        pthread_cond_destroy(&pCtx->cond);
      if ( oki > 0 )
      {
        pthread_mutex_destroy(&pCtx->lock);
      }
      break ; 
    }
    iok++ ; 
  } while(0) ; 
  if ( iok < 3 )
  {
    if ( iok > 1 )
    {
      ism_common_free(ism_memory_store_misc,pCtx );
      pCtx = NULL;
    }
    if ( iok > 0 )
    {
      if (genDir->path) ism_common_free(ism_memory_store_misc,genDir->path) ; 
      closedir(genDir->pdir) ; 
    }
  }
  pthread_mutex_unlock(&gLock) ; 
  return rc ; 
}
/*------------------------------------------------------*/
static int ism_storeDisk_placeJob(int type, ismStore_DiskTaskParams_t *params)
{
  int i;
  int rc = StoreRC_OK ;
  int iok=0 ; 
  ismStoe_DirInfo *di=NULL ; 
  ismStore_diskUtilsStoreJobInfo *job_info=NULL ; 
  ismStore_diskUtilsJob          *job ; 

  pthread_mutex_lock(&gLock) ; 
  do
  {
    if ((type == DUJOB_STORE_READ || type == DUJOB_STORE_WRITE) && 
        !(params->BufferParams->pBuffer && params->BufferParams->BufferLength) && 
        !(type == DUJOB_STORE_WRITE && params->BufferParams->fAllocMem) )
    {
      rc = StoreRC_BadParameter ; 
      break ; 
    }
    if ( type == DUJOB_STORE_COMP && !params->BufferParams->pBitMaps )
    {
      rc = StoreRC_BadParameter ; 
      break ; 
    }
    if (!params->Callback )
    {
      rc = StoreRC_BadParameter ; 
      break ; 
    }
    if (!pCtx || pCtx->goOn < 2 )
    {
      rc = StoreRC_Disk_IsNotOn ;
      break ; 
    }
    if ( params->Path && params->File )
      di = ism_storeDisk_getDirInfo(params->Path, params->File) ; 
    else
    if ( !params->Path && !params->File )
      di = genDir ; 
    else
      di = NULL ; 
    if ( !di )
    {
      rc = StoreRC_BadParameter ; 
      break ; 
    }
    if ( params->Priority < 0 || params->Priority >= DU_MAX_PRIO )
    {
      rc = StoreRC_BadParameter ; 
      break ; 
    }
    if ( type == DUJOB_STORE_WRITE && params->BufferParams->fAllocMem )
    {
      pthread_mutex_lock(&pCtx->lock) ; 
      for ( i=0, job=NULL ; i<DU_MAX_PRIO && !job ; i++ )
      {
        for ( job = pCtx->head[i] ; job ; job = job->next_job )
        {
          job_info = job->job_info ; 
          if ( job->job_type == type && job_info->GenId == params->GenId && job_info->BufferParams->fAllocMem )
          {
            if ( job->job_live )
              job->job_dead = 1 ; 
            else
              break ; 
          }
        }
      }
      pthread_mutex_unlock(&pCtx->lock) ; 
      if ( job )
      {
        rc = StoreRC_Disk_TaskExists ; 
        break ; 
      }
    }
    else
    if ( type == DUJOB_STORE_COMP )
    {
      pthread_mutex_lock(&pCtx->lock) ; 
      for ( i=0, job=NULL ; i<DU_MAX_PRIO && !job ; i++ )
      {
        for ( job = pCtx->head[i] ; job ; job = job->next_job )
        {
          job_info = job->job_info ; 
          if ( job->job_type == type && job_info->GenId == params->GenId && !job->job_live )
          {
            if ( params->Priority < job->job_prio )
              job->job_dead = 1 ; 
            else
            {
              int j ; 
              uint64_t **pBitMaps = job_info->BufferParams->pBitMaps ; 
              if ( job_info->BufferParams->fFreeMaps ) 
                for ( j=0 ; j<ismSTORE_GRANULE_POOLS_COUNT ; j++ ) if ( pBitMaps[j]) ism_common_free(ism_memory_store_misc,pBitMaps[j]) ; 
              ism_common_free(ism_memory_store_misc,pBitMaps) ; 
              job_info->BufferParams->pBitMaps = params->BufferParams->pBitMaps ; 
              job_info->BufferParams->fFreeMaps= params->BufferParams->fFreeMaps; 
              break ; 
            }
          }
        }
      }
      pthread_mutex_unlock(&pCtx->lock) ; 
      if ( job )
      {
        rc = StoreRC_Disk_TaskExists ; 
        break ; 
      }
    }
    else
    if ( type == DUJOB_STORE_DELETE )
    {
      pthread_mutex_lock(&pCtx->lock) ; 
      for ( i=0, job=NULL ; i<DU_MAX_PRIO && !job ; i++ )
      {
        for ( job = pCtx->head[i] ; job ; job = job->next_job )
        {
          job_info = job->job_info ; 
          if ( job_info->GenId == params->GenId )
            job->job_dead = 1 ; 
        }
      }
      pthread_mutex_unlock(&pCtx->lock) ; 
    }
    else
    if ( type == DUJOB_STORE_TERM )
    {
      pthread_mutex_lock(&pCtx->lock) ; 
      for ( i=0, job=NULL ; i<DU_MAX_PRIO && !job ; i++ )
      {
        for ( job = pCtx->head[i] ; job ; job = job->next_job )
        {
          if ( job->job_term )
            job->job_dead = 1 ; 
        }
      }
      pthread_mutex_unlock(&pCtx->lock) ; 
    }
    if (!(job_info = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,228),sizeof(ismStore_diskUtilsStoreJobInfo))) )
    {
      rc = StoreRC_AllocateError ;
      break ; 
    }
    iok++ ; 
    memset(job_info,0,sizeof(ismStore_diskUtilsStoreJobInfo)) ; 
    memcpy(job_info->BufferParams, params->BufferParams, sizeof(ismStore_DiskBufferParams_t)) ; 
    job_info->callback = params->Callback ; 
    job_info->pContext = params->pContext ; 
    job_info->di       = di ; 
    job_info->GenId    = params->GenId ; 
    if (!(job = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,229),sizeof(ismStore_diskUtilsJob))) )
    {
      rc = StoreRC_AllocateError ;
      break ; 
    }
    iok++ ; 
    memset(job,0,sizeof(ismStore_diskUtilsJob)) ; 
    job->job_info = job_info ; 
    job->job_type = type; 
    job->job_prio = params->Priority ; 
    job->job_term = params->fCancelOnTerm ; 
    i = job->job_prio ; 
    pthread_mutex_lock(&pCtx->lock) ; 
    if ( pCtx->tail[i] )
      pCtx->tail[i]->next_job = job ; 
    else
      pCtx->head[i] = job ; 
    pCtx->tail[i] = job ; 
    if ( type == DUJOB_STORE_TERM )
      pCtx->goOn = 1 ; 
    pthread_cond_signal(&pCtx->cond) ; 
    pthread_mutex_unlock(&pCtx->lock) ; 
    iok++ ; 
  } while(0) ; 
  if ( iok < 3 )
  {
    if ( di && di != genDir ) ism_storeDisk_delDirInfo(di) ; 
    if ( iok > 1 )
      ism_common_free(ism_memory_store_misc,job) ;
    if ( iok > 0 )
      ism_common_free(ism_memory_store_misc,job_info) ; 
  }
  pthread_mutex_unlock(&gLock) ; 
  return rc ; 
}
/*------------------------------------------------------*/
int ism_storeDisk_term(ismStore_DiskTaskParams_t *pDiskTaskParams)
{
  return ism_storeDisk_placeJob(DUJOB_STORE_TERM, pDiskTaskParams) ; 
}

/*------------------------------------------------------*/
int ism_storeDisk_writeGeneration(ismStore_DiskTaskParams_t *pDiskTaskParams)
{
  return ism_storeDisk_placeJob(DUJOB_STORE_WRITE, pDiskTaskParams) ; 
}
/*------------------------------------------------------*/
int ism_storeDisk_readGeneration(ismStore_DiskTaskParams_t *pDiskTaskParams)
{
  return ism_storeDisk_placeJob(DUJOB_STORE_READ, pDiskTaskParams) ; 
}
/*------------------------------------------------------*/
int ism_storeDisk_deleteGeneration(ismStore_DiskTaskParams_t *pDiskTaskParams)
{
  return ism_storeDisk_placeJob(DUJOB_STORE_DELETE, pDiskTaskParams) ; 
}
/*------------------------------------------------------*/
int ism_storeDisk_compactGeneration(ismStore_DiskTaskParams_t *pDiskTaskParams)
{
  return ism_storeDisk_placeJob(DUJOB_STORE_COMP, pDiskTaskParams) ; 
}

/*------------------------------------------------------*/

int ism_storeDisk_readGenerationsList(ismStore_DiskTaskParams_t *pDiskTaskParams)
{
  return ism_storeDisk_placeJob(DUJOB_STORE_LIST, pDiskTaskParams) ; 
}

/*------------------------------------------------------*/
int ism_storeDisk_writeFile(ismStore_DiskTaskParams_t *pDiskTaskParams)
{
  return ism_storeDisk_placeJob(DUJOB_STORE_WRITE, pDiskTaskParams) ; 
}
/*------------------------------------------------------*/
int ism_storeDisk_readFile(ismStore_DiskTaskParams_t *pDiskTaskParams)
{
  return ism_storeDisk_placeJob(DUJOB_STORE_READ, pDiskTaskParams) ; 
}
/*------------------------------------------------------*/
int ism_storeDisk_deleteDeadFiles(ismStore_DiskTaskParams_t *pDiskTaskParams) 
{
  return ism_storeDisk_placeJob(DUJOB_STORE_WIPE, pDiskTaskParams) ; 
}
/*------------------------------------------------------*/

int ism_storeDisk_getGenerationHeader(ismStore_GenId_t genId, uint32_t length, char *pBuffer)
{
  int rc ; 
  char fn[8] ; 
  ismStoe_DirInfo *di ; 
  ismStore_DiskBufferParams_t bp[1] ; 
  ismStore_diskUtilsStoreJobInfo job_info[1] ; 
  ismStore_diskUtilsJob          job[1] ; 

  di = genDir ; 
  memset(bp,0,sizeof(bp));
  bp->pBuffer = pBuffer ; 
  bp->BufferLength = length ; 

  pthread_mutex_lock(&gLock) ; 
    
  memset(job_info,0,sizeof(ismStore_diskUtilsStoreJobInfo)) ; 
  memcpy(job_info->BufferParams, bp, sizeof(ismStore_DiskBufferParams_t)) ; 
  job_info->di       = di ; 
  job_info->GenId    = genId ; 
  memset(job,0,sizeof(ismStore_diskUtilsJob)) ; 
  job->job_info = job_info ; 
  job->job_type = DUJOB_STORE_READ ; 
  snprintf(fn,8,"g%6.6u",job_info->GenId) ; 

  if ( (rc = ism_storeDisk_ioFile(fn, 1, job)) < 0 )
  {
    if ( rc == -4 ) 
      rc = StoreRC_Disk_TaskInterrupted  ; 
    else
    {
      rc = job->job_errno ; 
      TRACE(1,"%s failed with errno %d (%s) at %d\n",__FUNCTION__,rc,strerror(rc),job->job_line);
      rc = StoreRC_SystemError ; 
    }
  }

  pthread_mutex_unlock(&gLock) ; 
  return rc ; 
}
/*------------------------------------------------------*/
                    
int ism_storeDisk_getStatistics(ismStore_DiskStatistics_t *pDiskStats)
{
  int rc = StoreRC_OK ;
  pthread_mutex_lock(&gLock) ; 
  do
  {
    struct dirent *de ; 
    struct statfs sfs[1] ; 
    struct stat sf[1];
    if (!pCtx || pCtx->goOn < 2 )
    {
      rc = StoreRC_Disk_IsNotOn ;
      break ; 
    }
    if ( !pDiskStats )
    {
      rc = StoreRC_BadParameter ; 
      break ; 
    }
    memset(pDiskStats,0,sizeof(ismStore_DiskStatistics_t)) ; 

    if ( fstatfs(genDir->fdir, sfs) )
    {
      rc = StoreRC_SystemError ; 
      break ; 
    }
    pDiskStats->DiskFreeSpaceBytes = sfs->f_bfree * sfs->f_bsize ;
  //pDiskStats->DiskFreeSpaceBytes = sfs->f_bavail* sfs->f_bsize ;
    pDiskStats->DiskUsedSpaceBytes = (sfs->f_blocks - sfs->f_bfree) * sfs->f_bsize ;

    rewinddir(genDir->pdir);
    while ( (de = readdir(genDir->pdir)) )
    {
     #ifdef _DIRENT_HAVE_D_TYPE
      if ( de->d_type && de->d_type != DT_REG ) continue ; 
     #endif
      if ( !ism_store_isGenName(de->d_name) ) continue ; 
      if ( fstatat(genDir->fdir, de->d_name, sf, 0) )
      {
        rc = StoreRC_SystemError ; 
        break ; 
      }
      pDiskStats->NumGenerations++ ; 
      pDiskStats->GensUsedSpaceBytes += sf->st_size ;
    }
  } while(0) ;
  pthread_mutex_unlock(&gLock) ; 
  return rc ; 
}

/*------------------------------------------------------*/
int ism_storeDisk_getGenerationSize(ismStore_GenId_t genId, uint64_t *genSize)
{
  int rc = StoreRC_OK ;
  char fn[8];
  struct stat sf[1];
  pthread_mutex_lock(&gLock) ; 
  do
  {
    if (!pCtx || pCtx->goOn < 2 )
    {
      rc = StoreRC_Disk_IsNotOn ;
      break ; 
    }
    if ( !genSize )
    {
      rc = StoreRC_BadParameter ; 
      break ; 
    }
    snprintf(fn,8,"g%6.6u",genId) ; 
    if ( fstatat(genDir->fdir, fn, sf, 0) )
    {
      if ( errno == ENOENT )
       *genSize = 0 ; 
      else
        rc = StoreRC_SystemError ; 
      break ; 
    }
    *genSize = sf->st_size ; 
  } while(0) ;
  pthread_mutex_unlock(&gLock) ; 
  return rc ; 
}
/*------------------------------------------------------*/
int ism_storeDisk_getFileSize(const char *path, const char *file, uint64_t *fileSize)
{
  int rc = StoreRC_OK ;
  ismStoe_DirInfo *di=NULL ; 
  struct stat sf[1];
  pthread_mutex_lock(&gLock) ; 
  do
  {
    if (!pCtx || pCtx->goOn < 2 )
    {
      rc = StoreRC_Disk_IsNotOn ;
      break ; 
    }
    if ( !fileSize )
    {
      rc = StoreRC_BadParameter ; 
      break ; 
    }
    if (!(di = ism_storeDisk_getDirInfo(path, file)) )
    {
      rc = StoreRC_SystemError ; 
      break ; 
    }
    if ( fstatat(di->fdir, file, sf, 0) )
    {
      if ( errno == ENOENT )
       *fileSize = 0 ; 
      else
        rc = StoreRC_SystemError ; 
      break ; 
    }
    *fileSize = sf->st_size ; 
  } while(0) ;
  if ( di ) ism_storeDisk_delDirInfo(di) ; 
  pthread_mutex_unlock(&gLock) ; 
  return rc ; 
}

/*------------------------------------------------------*/

int ism_storeDisk_getGenerationInfo(ismStore_GenId_t genId, uint32_t length, char *pBuffer, uint64_t *pGenSize)
{
  int rc=StoreRC_BadParameter;

  if ( pGenSize && (rc = ism_storeDisk_getGenerationSize(genId, pGenSize)) != StoreRC_OK )
    return rc ; 
  if ( pBuffer )
    rc = ism_storeDisk_getGenerationHeader(genId, length, pBuffer) ; 
  return rc ; 
}

/*------------------------------------------------------*/

int ism_storeDisk_compactGenerationData(void *genData, ismStore_DiskBufferParams_t *bp)
{
  ismStore_memGenHeader_t *pGenHeader = (ismStore_memGenHeader_t *)genData ; 
  if ( pGenHeader->CompactSizeBytes )
  {
    bp->pBuffer = genData ; 
    bp->BufferLength = pGenHeader->CompactSizeBytes ; 
    return ism_storeDisk_reflateMemGen(bp->pBitMaps, bp) ; 
  }
  else
    return ism_storeDisk_deflateMemGen(genData, bp->pBitMaps, bp) ; 
}

/*------------------------------------------------------*/
int ism_storeDisk_expandGenerationData(void *genData, ismStore_DiskBufferParams_t *pBufferParams)
{
  size_t DS, off;
  ismStore_memGenHeader_t *pGenHeader ;
  ismStore_memDescriptor_t *desc ; 
  ismStore_memGranulePool_t *pPool;
  char *bptr, *eptr , *tptr ; 

  pGenHeader = (ismStore_memGenHeader_t *)genData ;
  if ( pGenHeader->MemSizeBytes != pBufferParams->BufferLength )
  {
    TRACE(1," Gen Size Mismatch: pGenHeader->MemSizeBytes (%lu) != pBufferParams->BufferLength (%lu)\n",
              pGenHeader->MemSizeBytes, pBufferParams->BufferLength);
    return StoreRC_BadParameter ; 
  }

  DS = pGenHeader->DescriptorStructSize ; 
  off = pGenHeader->GranulePool[0].Offset ; 
  bptr = (char *)pGenHeader + off ; 
  eptr = (char *)pGenHeader + pGenHeader->CompactSizeBytes ; 

  tptr = pBufferParams->pBuffer ; 
  memcpy(tptr, pGenHeader, off) ;
  if ( pBufferParams->fClearTarget ) 
    memset(tptr+off,0,pGenHeader->MemSizeBytes-off) ; 

  while ( bptr < eptr )
  {
    desc = (ismStore_memDescriptor_t *)bptr ; 
    pPool = &pGenHeader->GranulePool[desc->PoolId] ; 
    off = pPool->Offset + (size_t)desc->GranuleIndex * pPool->GranuleSizeBytes ; 
    memcpy(tptr+off, bptr , ALIGNED(DS+desc->DataLength)) ; 
    bptr += ALIGNED(DS+desc->DataLength) ; 
    if ( desc->DataType == 0 )
      desc->DataLength = 0 ; 
  }
  pGenHeader = (ismStore_memGenHeader_t *)tptr ; 
  pGenHeader->CompactSizeBytes =0 ; 
  return StoreRC_OK ;
}
/*------------------------------------------------------*/
int ism_storeDisk_removeCompactTasks(void)
{
  int i ; 
  ismStore_diskUtilsJob          *job ;

  pthread_mutex_lock(&pCtx->lock) ; 
  for ( i=0 ; i<DU_MAX_PRIO ; i++ )
    for ( job = pCtx->head[i] ; job ; job = job->next_job )
      if ( job->job_type == DUJOB_STORE_COMP || 
          (job->job_type == DUJOB_STORE_WRITE && ((ismStore_diskUtilsStoreJobInfo *)job->job_info)->BufferParams->fAllocMem) )
        job->job_dead = 1 ; 
  pthread_mutex_unlock(&pCtx->lock) ; 
  return StoreRC_OK ;
}
/*------------------------------------------------------*/
int ism_storeDisk_compactTasksCount(int priority)
{
  int i, n;
  ismStore_diskUtilsJob          *job ;

  i = priority ; 
  n = 0 ; 
  pthread_mutex_lock(&pCtx->lock) ; 
  for ( job = pCtx->head[i] ; job ; job = job->next_job )
    if ( job->job_type == DUJOB_STORE_COMP ) n++ ; 
  pthread_mutex_unlock(&pCtx->lock) ; 
  return n ; 
}
/*------------------------------------------------------*/
/*------------------------------------------------------*/
static ismStore_diskUtilsJob *nextJob( int prio)
{
  for ( int i=0 ; i<prio && i<DU_MAX_PRIO ; i++ ) if ( pCtx->head[i] ) return pCtx->head[i] ; 
  return NULL ; 
}
/*------------------------------------------------------*/
static int ism_storeDisk_ioFile(char *fn, int ioIn, ismStore_diskUtilsJob *job) 
{
  int f, fd ; 
  ssize_t bytes ; 
  char *bptr, *eptr ; 
  void *buff ; 
  size_t batch, count=0;
  char tfn[64], *pfn ; 
  ismStoe_DirInfo *di ; 
  ismStore_diskUtilsStoreJobInfo *job_info = (ismStore_diskUtilsStoreJobInfo *)job->job_info ;

  di = job_info->di ; 
  batch = di->batch ; 
  if ( batch > job_info->BufferParams->BufferLength )
       batch = (job_info->BufferParams->BufferLength+di->block-1)/di->block*di->block ; 
  bytes = posix_memalign(&buff, di->block, batch) ; 
  if ( bytes )
  {
    job->job_errno = bytes ; 
    job->job_line = __LINE__ ;
    return -1 ; 
  }

  f = ioIn ? O_RDONLY : (O_WRONLY | O_CREAT) ;
  f |=O_NOATIME ;
  f |=O_CLOEXEC;
  if ( !(job_info->BufferParams->BufferLength % di->block) )
    f |=O_DIRECT ;  
  if ( ioIn )
    pfn = fn ; 
  else
  {
    snprintf(tfn,64,"tid_%lu",pCtx->tid) ; 
    pfn = tfn ; 
  }
  if ( (fd = openat(di->fdir,pfn,f,ismStore_memGlobal.PersistedFileMode)) < 0 )
  {
    //if ( errno == EINVAL && di != genDir )
    if ( errno == EINVAL )  // allow non-direct for virtual appliances  
    {
      f &= ~(O_DIRECT) ; 
      fd = openat(di->fdir,pfn,f,ismStore_memGlobal.PersistedFileMode) ;
    }
    if ( fd < 0 )
    {
      job->job_errno = errno ; 
      job->job_line = __LINE__ ;
      return -1 ; 
    }
  }

  if ( ioIn )
  {
    struct stat sf[1];
    if ( fstat(fd,sf) < 0 )
    {
      close(fd) ; 
      job->job_errno = errno ; 
      job->job_line = __LINE__ ;
      return -1 ; 
    }
    if ( job_info->BufferParams->BufferLength > sf->st_size )
      job_info->BufferParams->BufferLength = sf->st_size ;
  }
  bptr = job_info->BufferParams->pBuffer ; 
  eptr = bptr + (uintptr_t)job_info->BufferParams->BufferLength ; 

  while ( bptr < eptr && pCtx->goOn && !job->job_dead && !nextJob(job->job_prio) )
  {
    if ( batch > (size_t)(eptr - bptr) )
      batch = (size_t)(eptr - bptr) ;
    ism_common_going2work();
    if ( ioIn )
    {
      bytes = read(fd, buff, batch) ;
      memcpy(bptr,buff,batch);
    }
    else
    {
      memcpy(buff,bptr,batch);
      bytes = write(fd, buff, batch) ; 
    }
    ism_common_backHome();
    if ( bytes != batch )
    {
      int ok=0 ; 
      do
      {
        if ( ++count > 5000 )
        {
          job->job_errno = EIO ; 
          job->job_line = __LINE__ ; 
          break ; 
        }
        if ( bytes > 0 )
        {
          if ( ioIn )
          {
            if ( read(fd, buff, 1) == 0 )
            {
              job->job_errno = EIO ; 
              job->job_line = __LINE__ ; 
              break ; 
            }
          }
          else
          {
            if ( write(fd, buff, 1) < 0 )
            {
              job->job_errno = errno ; 
              job->job_line = __LINE__ ; 
              break ; 
            }
          }
          bytes++ ; 
          if ( lseek(fd, -bytes, SEEK_CUR) == (off_t)(-1) )
          {
            job->job_errno = errno ; 
            job->job_line = __LINE__ ; 
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
            job->job_errno = errno ; 
            job->job_line = __LINE__ ; 
            //printf("_dbg_%s_%d: ioIn=%d, fd=%d, bptr=%p, batch=%lu, pBuf=%p, len=%lu\n",__FUNCTION__,__LINE__,ioIn,fd,bptr,batch,job_info->BufferParams->pBuffer,job_info->BufferParams->BufferLength);
            break ; 
          }
        }
        ok++ ; 
      } while(0) ; 
      if ( !ok )
      {
        close(fd) ; 
        return -1 ; 
      }
      su_sleep(4,SU_MIL);
    }
    bptr += bytes ; 
  }
  ism_common_free_memaligned(ism_memory_store_misc,buff);
  if ( bptr < eptr )
  {
    job->job_errno = errno ; 
    job->job_line = __LINE__ ; 
    close(fd) ;
    return -4 ; 
  }
  if ( !ioIn )
  {
    if ( ftruncate(fd,job_info->BufferParams->BufferLength) != 0 ) 
    {
      TRACE(1,"Could not truncate: %s\n",pfn);
    }
  }
  if ( !ioIn && fsync(fd) )
  {
    job->job_errno = errno ; 
    job->job_line = __LINE__ ; 
    close(fd) ; 
    return -2 ; 
  }
  if ( close(fd) )
  {
    job->job_errno = errno ; 
    job->job_line = __LINE__ ; 
    return -3 ; 
  }
  if ( pfn != fn )
  {
    int rc;
    pthread_mutex_lock(&gLock) ; 
    rc = renameat(di->fdir, pfn, di->fdir, fn) ; 
    pthread_mutex_unlock(&gLock) ; 
    if ( rc < 0 )
    {
      job->job_errno = errno ; 
      job->job_line = __LINE__ ; 
      return -5 ; 
    }
  }
  return StoreRC_OK ;
}
/*------------------------------------------------------*/

static void *ism_store_diskUtilsThread(void *arg, void * context, int value)
{
  int rc , myInd , redo=0, ilock;
  ismStore_diskUtilsJob *job=NULL ; 
  
  myInd = value ; 
  pCtx->thUp = 1 ; 
  TRACE(5, "The %s thread is started (%d)\n", __FUNCTION__,myInd);

  for(;;)
  {
    ilock = 0 ; 
    if ( job )
    {
      if ( redo )
        job->job_live  = 0 ; 
      else
      {
        ismStore_diskUtilsStoreJobInfo *job_info = (ismStore_diskUtilsStoreJobInfo *)job->job_info ; 
        int i = job->job_prio ; 
        ism_storeDisk_delDirInfo(job_info->di) ; 
        ilock++ ; 
        pthread_mutex_lock(&pCtx->lock) ; 
        if (!(pCtx->head[i] = job->next_job) )
          pCtx->tail[i] = NULL ; 
        ism_common_free(ism_memory_store_misc,job_info);
        ism_common_free(ism_memory_store_misc,job);
      }
    }
    if ( !ilock )
    {
      pthread_mutex_lock(&pCtx->lock) ; 
    }
    ism_common_backHome();
    if ( !pCtx->goOn )
    {
      pthread_mutex_unlock(&pCtx->lock) ; 
      break ; 
    }
    while (!(job = nextJob(DU_MAX_PRIO)) )
    {
      if ( !pCtx->goOn )
      {
        pthread_mutex_unlock(&pCtx->lock) ; 
        break ; 
      }
      pthread_cond_wait(&pCtx->cond, &pCtx->lock) ; 
    }
    ism_common_going2work();
    if ( job )
    {
      char fn[8], *pfn ; 
      ismStore_DiskGenInfo_t dgi[1] ; 
      ismStore_diskUtilsStoreJobInfo *job_info = (ismStore_diskUtilsStoreJobInfo *)job->job_info ; 
      ismStoe_DirInfo   *di = job_info->di ; 
      job->job_errno = 0 ; 
      job->job_live  = 1 ; 
      rc = StoreRC_OK ;
      pthread_mutex_unlock(&pCtx->lock) ; 
      if ( di->file )
        pfn = di->file ; 
      else
      {
        snprintf(fn,8,"g%6.6u",job_info->GenId) ; 
        pfn = fn ; 
      }
      redo = 0 ; 
      if ( job->job_dead )
      {
        if ( job->job_type == DUJOB_STORE_COMP )
        {
          int i ; 
          uint64_t **pBitMaps = job_info->BufferParams->pBitMaps ; 
          if ( job_info->BufferParams->fFreeMaps ) 
            for ( i=0 ; i<ismSTORE_GRANULE_POOLS_COUNT ; i++ ) if ( pBitMaps[i]) ism_common_free(ism_memory_store_misc,pBitMaps[i]) ; 
          ism_common_free(ism_memory_store_misc,pBitMaps) ; 
        }
        TRACE(5, "%s: A disk task was cancelled: task=%d , file=%s/%s\n",__FUNCTION__, job->job_type,di->path,pfn) ; 
        job_info->callback(job_info->GenId, StoreRC_Disk_TaskCancelled, NULL, job_info->pContext);
      }
      else
      if ( job->job_type >= DUJOB_STORE_TERM &&
           job->job_type <= DUJOB_STORE_DELETE )
      {
        double dt=su_sysTime(), dl=1e0;
        if ( job->job_type == DUJOB_STORE_WRITE ||
             job->job_type == DUJOB_STORE_READ )
        {
          int ioIn ;
          if ( job->job_type == DUJOB_STORE_READ )
            ioIn = 1 ; 
          else
          {
            ioIn = 0 ; 
            if ( job_info->BufferParams->fAllocMem )
            {
              rc = ism_store_memRecoveryGetGeneration(job_info->GenId, job_info->BufferParams) ; 
              job->job_errno = errno ; 
              job->job_line = __LINE__ ; 
            }
          }
          if ( rc == StoreRC_OK && (rc = ism_storeDisk_ioFile(pfn, ioIn, job)) < 0 )
          {
            if ( rc == -4 ) 
            {
              if ( pCtx->goOn && !job->job_dead && nextJob(job->job_prio) )
                redo = 1 ; 
              rc = StoreRC_Disk_TaskInterrupted  ; 
            }
            else
              rc = StoreRC_SystemError ; 
          }
          if ( job->job_type == DUJOB_STORE_WRITE && job_info->BufferParams->fAllocMem && job_info->BufferParams->pBuffer )
          {
            ism_common_free_memaligned(ism_memory_store_misc,job_info->BufferParams->pBuffer) ;
            job_info->BufferParams->pBuffer = NULL ; 
          }
          dgi->DataLength = job_info->BufferParams->BufferLength ; 
          dl = dgi->DataLength ; 
        }
        else
        if ( job->job_type == DUJOB_STORE_DELETE )
        {
          if ( unlinkat(di->fdir,pfn,0) )
          {
            job->job_errno = errno ; 
            job->job_line = __LINE__ ; 
            rc = StoreRC_SystemError ; 
          }
        }
        else
        if ( job->job_type == DUJOB_STORE_TERM )
          pCtx->goOn = 0 ; 

        if ( !redo )
        {
          if ( rc != StoreRC_OK )
          {
            TRACE(1, "%s: A disk task did not complete successfully: task=%d , file=%s/%s, errno=%d (%s), place=%d, rc=%d\n",__FUNCTION__, job->job_type,di->path,pfn,job->job_errno,strerror(job->job_errno),job->job_line,rc);
          }
          else
          {
            dt = su_sysTime()-dt;
            TRACE(5, "%s: A disk task completed successfully: task=%d, priority=%d, file=%s/%s, size=%f, time=%f sec, rate=%f Bytes/sec\n",__FUNCTION__,job->job_type,job->job_prio,di->path,pfn,dl,dt,dl/dt) ; 
          }
          job_info->callback(job_info->GenId, rc, dgi, job_info->pContext);
        }
      }
      else
      if ( job->job_type == DUJOB_STORE_COMP )
      {
        void *genData ; 
        ismStore_DiskBufferParams_t bp[1] ;
        ismStore_diskUtilsStoreJobInfo tji[1]; 
        ismStore_diskUtilsJob          tj[1] ; 
        ismStore_memGenHeader_t *pGenHeader ; 
        size_t os=0, ns=0 ; 

        memset(bp,0,sizeof(ismStore_DiskBufferParams_t)) ; 
        snprintf(fn,8,"g%6.6u",job_info->GenId) ; 
        pfn = fn ; 
        do
        {
          void *p ;
          if ( (rc = ism_storeDisk_getGenerationSize(job_info->GenId, &bp->BufferLength)) != StoreRC_OK )
          {
            job->job_errno = errno ; 
            job->job_line = __LINE__ ; 
            break ; 
          }
          os = bp->BufferLength ; 
          if ( (rc = posix_memalign(&p,ismStore_memGlobal.DiskBlockSizeBytes, bp->BufferLength)) != 0 )
          {
            job->job_errno = rc ; 
            job->job_line = __LINE__ ; 
            rc = StoreRC_AllocateError ;
            break ; 
          }
          bp->pBuffer = p ; 
          memset(tji, 0, sizeof(ismStore_diskUtilsStoreJobInfo)) ; 
          memset(tj , 0, sizeof(ismStore_diskUtilsJob)) ; 
          memcpy(tji->BufferParams, bp, sizeof(ismStore_DiskBufferParams_t)) ;
          tji->di = genDir ; 
          tji->GenId = job_info->GenId ;
          tj->job_info = tji ; 
          tj->job_type = DUJOB_STORE_READ ; 
          tj->job_prio = job->job_prio ; 
          if ( (rc = ism_storeDisk_ioFile(pfn, 1, tj)) < 0 )
          {
            job->job_errno = tj->job_errno ; 
            job->job_line  = tj->job_line ; 
            if ( rc == -4 ) 
            {
              if ( pCtx->goOn && !job->job_dead && nextJob(job->job_prio) )
                redo = 1 ; 
              rc = StoreRC_Disk_TaskInterrupted  ; 
            }
            else
              rc = StoreRC_SystemError ; 
            break ; 
          }
          memcpy(bp, tji->BufferParams, sizeof(ismStore_DiskBufferParams_t)) ;
          pGenHeader = (ismStore_memGenHeader_t *)bp->pBuffer ; 
          if ( pGenHeader->CompactSizeBytes )
          {
            rc = ism_storeDisk_reflateMemGen(job_info->BufferParams->pBitMaps, bp) ; 
            job->job_line = __LINE__ ; 
          }
          else
          {
            genData = bp->pBuffer ; 
            bp->pBuffer = NULL ; 
            rc = ism_storeDisk_deflateMemGen(genData, job_info->BufferParams->pBitMaps, bp) ; 
            job->job_line = __LINE__ ; 
            ism_common_free(ism_memory_store_misc,genData) ; 
          }
          if ( rc != StoreRC_OK )
          {
            job->job_line = __LINE__ ; 
            break ; 
          }
          dgi->StdDev = bp->StdDev ; 
          ns = bp->BufferLength ; 
          memset(tji, 0, sizeof(ismStore_diskUtilsStoreJobInfo)) ; 
          memset(tj , 0, sizeof(ismStore_diskUtilsJob)) ; 
          memcpy(tji->BufferParams, bp, sizeof(ismStore_DiskBufferParams_t)) ;
          tji->di = genDir ; 
          tji->GenId = job_info->GenId ;
          tj->job_info = tji ; 
          tj->job_type = DUJOB_STORE_WRITE ; 
          tj->job_prio = job->job_prio ; 
          if ( (rc = ism_storeDisk_ioFile(pfn, 0, tj)) < 0 )
          {
            job->job_errno = tj->job_errno ; 
            job->job_line  = tj->job_line ; 
            if ( rc == -4 ) 
            {
              if ( pCtx->goOn && !job->job_dead && nextJob(job->job_prio) )
                redo = 1 ; 
              rc = StoreRC_Disk_TaskInterrupted  ; 
            }
            else
              rc = StoreRC_SystemError ; 
            break ; 
          }
          dgi->DataLength = ns ; 
        } while (0) ; 
        if ( !redo )
        {
          int i ; 
          uint64_t **pBitMaps = job_info->BufferParams->pBitMaps ; 
          if ( job_info->BufferParams->fFreeMaps ) 
            for ( i=0 ; i<ismSTORE_GRANULE_POOLS_COUNT ; i++ ) if ( pBitMaps[i]) ism_common_free(ism_memory_store_misc,pBitMaps[i]) ; 
          ism_common_free(ism_memory_store_misc,pBitMaps) ; 
          if ( rc != StoreRC_OK )
          {
            TRACE(1, "%s: A disk task did not complete successfully: task=%d , file=%s/%s, errno=%d (%s), place=%d\n",__FUNCTION__, job->job_type,di->path,pfn,job->job_errno,strerror(job->job_errno),job->job_line);
          }
          else
          {
            TRACE(5, "%s: A disk task completed successfully: task=%d, priority=%d, file=%s/%s ; oldSize=%lu, newSize=%lu\n",__FUNCTION__, job->job_type,job->job_prio,di->path,pfn,os,ns) ; 
          }
          job_info->callback(job_info->GenId, rc, dgi, job_info->pContext);
        }
        if ( bp->pBuffer) ism_common_free_memaligned(ism_memory_store_misc,bp->pBuffer) ;
      }
      else
      if ( job->job_type == DUJOB_STORE_LIST )
      {
        size_t len ; 
        ismStore_DiskGenInfo_t *f1st=NULL, *cur ; 
        struct dirent *de ; 
        char *ep;
        len = sizeof(ismStore_DiskGenInfo_t) + job_info->BufferParams->BufferLength ; 
        pthread_mutex_lock(&gLock) ; 
        rewinddir(di->pdir);
        while ( (de = readdir(di->pdir)) )
        {
          int gid;
         #ifdef _DIRENT_HAVE_D_TYPE
          if ( de->d_type && de->d_type != DT_REG ) continue ; 
         #endif
          if ( !ism_store_isGenName(de->d_name) ) continue ; 
          gid = strtol(de->d_name+1,&ep,10);
          if ( !ep || *ep ) continue ; 
          if ( !(cur = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,242),len)) )
          {
            job->job_errno = errno ; 
            job->job_line = __LINE__ ; 
            rc = StoreRC_SystemError ; 
            break ; 
          }
          memset(cur,0,len) ; 
          cur->DataLength = job_info->BufferParams->BufferLength ; 
          ep = NULL ; 
          cur->GenId = gid ;
          job_info->BufferParams->pBuffer = cur->Data ; 
          if ( (rc = ism_storeDisk_ioFile(de->d_name, 1, job)) < 0 )
          {
            if ( rc == -4 ) 
            {
              if ( pCtx->goOn && !job->job_dead && nextJob(job->job_prio) )
                redo = 1 ; 
              rc = StoreRC_Disk_TaskInterrupted  ; 
            }
            else
              rc = StoreRC_SystemError ; 
            ism_common_free(ism_memory_store_misc,cur) ;
            break ; 
          }
          cur->pNext = f1st ; 
          f1st = cur ; 
        }
        pthread_mutex_unlock(&gLock) ; 
        if ( !redo )
        {
          if ( rc != StoreRC_OK )
          {
            TRACE(1, "%s: A disk task did not complete successfully: task=%d , errno=%d (%s), place=%d\n",__FUNCTION__, job->job_type,job->job_errno,strerror(job->job_errno),job->job_line);
          }
          job_info->callback(job_info->GenId, rc,f1st,job_info->pContext);
        }
        while( f1st )
        {
          cur = f1st ; 
          f1st = cur->pNext ; 
          ism_common_free(ism_memory_store_misc,cur) ; 
        }
      }
      else
      if ( job->job_type == DUJOB_STORE_WIPE )
      {
        ismStore_memMgmtHeader_t *pMgmHeader;
        ismStore_Handle_t handle ; 
        ismStore_memDescriptor_t *desc ;
        ismStore_memGenIdChunk_t *chunk ;
        register size_t DS;
        struct dirent *de ; 
        char gok[65536];
      
        rc = StoreRC_OK ; 
        pMgmHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress;
        DS = pMgmHeader->DescriptorStructSize ; 
        memset(gok,0,sizeof(gok)) ; 
        for ( handle=pMgmHeader->GenIdHandle ; handle ; handle=desc->NextHandle ) 
        {
          desc = (ismStore_memDescriptor_t *)(ismStore_memGlobal.pStoreBaseAddress + ismSTORE_EXTRACT_OFFSET(handle)) ;
          chunk = (ismStore_memGenIdChunk_t *)((uintptr_t)desc+DS) ;
          for ( int i=0 ; i<chunk->GenIdCount ; i++ )
            gok[chunk->GenIds[i]] = 1 ; 
        }
        pthread_mutex_lock(&gLock) ; 
        rewinddir(di->pdir);
        while ( (de = readdir(di->pdir)) )
        {
         #ifdef _DIRENT_HAVE_D_TYPE
          if ( de->d_type && de->d_type != DT_REG ) continue ; 
         #endif
          if ( ism_store_isGenName(de->d_name) )
          {
            char *ep=NULL ; 
            int id = strtol(de->d_name+1,&ep,10);
            if ( !ep || *ep ) continue ; 
            if (!gok[id] )
            {
              if ( unlinkat(di->fdir, de->d_name,0) )
              {
                job->job_errno = errno ; 
                job->job_line = __LINE__ ; 
                rc = StoreRC_SystemError ; 
                break ; 
              }
              else
              {
                TRACE(5,"A disk file for an orphan generation has been deleted: %s/%s\n",di->path,de->d_name) ; 
              }
            }
          }
        }
        pthread_mutex_unlock(&gLock) ; 
        if ( rc != StoreRC_OK )
        {
          TRACE(1, "%s: A disk task did not complete successfully: task=%d , errno=%d (%s), place=%d\n",__FUNCTION__, job->job_type,job->job_errno,strerror(job->job_errno),job->job_line);
        }
        job_info->callback(job_info->GenId, rc,NULL,job_info->pContext);
      }
    }
    else
    {
      pthread_mutex_unlock(&pCtx->lock) ; 
    }
  }

  pthread_mutex_lock(&pCtx->lock) ; 
  for ( int i=0 ; i<DU_MAX_PRIO ; i++ )
  {
    while( (job = pCtx->head[i]) )
    {
      ismStore_diskUtilsStoreJobInfo *job_info = (ismStore_diskUtilsStoreJobInfo *)job->job_info ;
      job_info->callback(job_info->GenId, StoreRC_Disk_TaskCancelled, NULL, job_info->pContext);
      if ( !(pCtx->head[i] = job->next_job) ) 
        pCtx->tail[i] = NULL ; 
      ism_common_free(ism_memory_store_misc,job->job_info);
      ism_common_free(ism_memory_store_misc,job);
    }
  }
  pthread_mutex_unlock(&pCtx->lock) ; 

  ism_common_detachThread(pCtx->tid);
  pCtx->thUp = 0 ; 

  pthread_cond_destroy(&pCtx->cond);
  pthread_mutex_destroy(&pCtx->lock);

  pthread_mutex_lock(&gLock) ; 
  {
    closedir(genDir->pdir) ; 
    ism_common_free(ism_memory_store_misc,genDir->path) ;
    ism_common_free(ism_memory_store_misc,pCtx);
    pCtx = NULL;
  }
  pthread_mutex_unlock(&gLock) ; 

  //ism_common_endThread(NULL);
  return NULL;
}


#ifdef __cplusplus
}
#endif

/*********************************************************************/
/* End of storeDiskUtils.c                                           */
/*********************************************************************/
