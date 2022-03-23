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
/* Module Name: storeDiskUtils.h                                     */
/*                                                                   */
/* Description: Store disk utilities header file                     */
/*                                                                   */
/*********************************************************************/
#ifndef __ISM_STORE_DISKUTILS_DEFINED
#define __ISM_STORE_DISKUTILS_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include <dirent.h>
#include <sys/uio.h>
#include "store.h"            /* Store external header file          */
#include "storeInternal.h"

typedef struct ismStore_DiskParameters_t
{
   size_t              TransferBlockSize;      /* Preferred read/write block size                  */
   size_t              DiskBlockSize;          /* Disk preferred block size (output parameter)     */
   char                RootPath[PATH_MAX + 1]; /* Root directory for stored generation files       */
   uint8_t             ClearStoredFiles;       /* If set (value != 0) the stored files will be
                                                * deleted during startup.                          */
} ismStore_DiskParameters_t;

typedef struct ismStore_DiskBufferParams_t
{
   char             *pBuffer;               /* Pointer to the buffer                               */
   uint64_t        **pBitMaps;              /* Pointer to an array of granule bitmaps              */
   uint64_t          BufferLength;          /* Length of the buffer                                */
   uint64_t          StdDev;                /* Standard Deviation of kept records                  */
   uint8_t           fAllocMem;
   uint8_t           fFreeMaps;
   uint8_t           fClearTarget ; 
   uint8_t           fCompactRefChunks ; 
} ismStore_DiskBufferParams_t;

typedef struct ismStore_DiskStatistics_t
{
   uint32_t            NumGenerations;     /* Number of generations stored on the disk             */
   uint64_t            DiskFreeSpaceBytes; /* Disk free space in bytes                             */
   uint64_t            DiskUsedSpaceBytes; /* Disk used space in bytes                             */
   uint64_t            GensUsedSpaceBytes; /* Disk used space in bytes                             */
} ismStore_DiskStatistics_t;

typedef struct ismStore_DiskGenInfo_t
{
   struct ismStore_DiskGenInfo_t *pNext;   /* Pointer to the next generation element               */
   uint64_t            DataLength;         /* Data length                                          */
   uint64_t            StdDev;             /* Standard Deviation of kept records                   */
   ismStore_GenId_t    GenId;              /* Generation Id                                        */
   char                Data[1];            /* Data                                                 */
} ismStore_DiskGenInfo_t;


typedef struct ismStoe_DirInfo
{
  struct ismStoe_DirInfo  *next ; 
  DIR                     *pdir ; 
  char                    *path ;
  char                    *file ;
  size_t                   block;
  size_t                   batch;
  int                      fdir ; 
  int                      plen ; 
} ismStoe_DirInfo ; 

/*********************************************************************/
/* Callback structure used to specify the callback function and      */
/* argument used to indicate completion of an asynchronous           */
/* store disk operation. The callback has four arguments:            */
/*                                                                   */
/* 1. The generation Id.                                             */
/* 2. The return code for the operation.                             */
/* 3. Pointer to the generation info (used only in                   */
/*    ism_storeDisk_readGenerationsList API)                         */
/* 4. The context provided when the callback was registered.         */
/*********************************************************************/
typedef void (*ismPSTOREDISKASYNCCALLBACK)(ismStore_GenId_t genId,
                                           int32_t retcode,
                                           ismStore_DiskGenInfo_t *pDiskGenInfo,
                                           void *pContext);

typedef struct ismStore_DiskTaskParams_t
{
   uint8_t                       fCancelOnTerm ; 
   uint8_t                       Priority ; 
   ismStore_GenId_t              GenId ; 
   ismPSTOREDISKASYNCCALLBACK    Callback ; 
   void                         *pContext ; 
   ismStore_DiskBufferParams_t   BufferParams[1] ; 
   const char                   *Path ; //This might have a variable like ${IMA_SVR_DATA_PATH} in
   const char                   *File ; 
   const char                   *ActualPath ; //The version of Path with vars replaced
} ismStore_DiskTaskParams_t;

/*********************************************************************/
/* Store Init                                                        */
/*                                                                   */
/* Performs store disk init tasks                                    */
/*                                                                   */
/* @param pStoreDiskParams   Store disk parameters                   */
/* @return A return code, StoreRC_OK=success                  */
/*********************************************************************/
int ism_storeDisk_init(ismStore_DiskParameters_t *pStoreDiskParams);

/*********************************************************************/
/* Store Shutdown                                                    */
/*                                                                   */
/* Performs shutdown tasks                                           */
/* The shutdown is done asynchronously and the callback function     */
/* notifies the user when the operation has been completed.          */
/*                                                                   */
/* @param fCompletePending  If set (value != 0) the store disk       */
/* module should first complete all pending tasks and only then shut */
/* down. If not set, all pending tasks are aborted and shut down is  */
/* performed immediately.                                            */
/* @param callback          Callback function supplied with the data */
/* @param pContext          Context to associate with the callback   */
/* @param threadNumber      Thread number                            */
/*                                                                   */
/* @return A return code, StoreRC_OK=success                  */
/*********************************************************************/
int ism_storeDisk_term(ismStore_DiskTaskParams_t *pDiskTaskParams) ; 

/*********************************************************************/
/* Read generations list from the disk                               */
/*                                                                   */
/* Read the list of generations which are stored on the disk.        */
/* Read is done asynchronously and the callback function notifies    */
/* the user when the operation has been completed.                   */
/* If the operation failed the callback function provides a reason   */
/* code and a description of the problem (e.g., disk error)          */
/*                                                                   */
/* @param length        Number of bytes to read from each generation */
/* @param callback      Callback function supplied with the data     */
/* @param pContext      Context to associate with the callback       */
/* @param threadNumber  Thread number                                */
/*                                                                   */
/* @return A return code, StoreRC_OK=success                  */
/*********************************************************************/
int ism_storeDisk_readGenerationsList(ismStore_DiskTaskParams_t *pDiskTaskParams) ;

/*********************************************************************/
/* Get Generation information                                        */
/*                                                                   */
/* @param genId    The generation Id                                 */
/* @param length   Number of bytes to read from the generation file. */
/*                 Value of zero means that no bytes should be read  */
/* @param pBuffer  Pointer to the buffer to which the data will be   */
/*                 copied (output)                                   */
/* @param pGenSize Pointer to the generation file size (output)      */
/*                                                                   */
/* @return A return code, StoreRC_OK=success                  */
/*********************************************************************/
int ism_storeDisk_getGenerationInfo(ismStore_GenId_t genId,
                                    uint32_t length,
                                    char *pBuffer,
                                    uint64_t *pGenSize);
int ism_storeDisk_getGenerationHeader(ismStore_GenId_t genId, uint32_t length, char *pBuffer);
int ism_storeDisk_getGenerationSize(ismStore_GenId_t genId, uint64_t *genSize);

/*********************************************************************/
/* Write store generation data to the disk                           */
/*                                                                   */
/* Write a generation that is stored in memory to disk.              */
/* Write is done asynchronously and the callback function notifies   */
/* the user when the operation has been completed.                   */
/* If the operation failed the callback function provides a reason   */
/* code and a description of the problem (e.g., out of disk space)   */
/*                                                                   */
/* @param genId         The generation Id                            */
/* @param pBufferParams The disk buffer parameters                   */
/* @param callback      Callback function supplied with the data     */
/* @param pContext      Context to associate with the callback       */
/* @param threadNumber  Thread number                                */
/*                                                                   */
/* @return A return code, StoreRC_OK=success                  */
/*********************************************************************/
int ism_storeDisk_writeGeneration(ismStore_DiskTaskParams_t *pDiskTaskParams) ;

/*********************************************************************/
/* Read store generation data from the disk                          */
/*                                                                   */
/* Read a generation that is stored on the disk to the memory.       */
/* Read is done asynchronously and the callback function notifies    */
/* the user when the operation has been completed.                   */
/* If the operation failed the callback function provides a reason   */
/* code and a description of the problem (e.g., disk error)          */
/*                                                                   */
/* @param genId         The generation Id                            */
/* @param pBufferParams The disk buffer parameters                   */
/* @param callback      Callback function supplied with the data     */
/* @param pContext      Context to associate with the callback       */
/* @param threadNumber  Thread number                                */
/*                                                                   */
/* @return A return code, StoreRC_OK=success                  */
/*********************************************************************/
int ism_storeDisk_readGeneration(ismStore_DiskTaskParams_t *pDiskTaskParams) ;

/*********************************************************************/
/* Compact store generation file on the disk                         */
/*                                                                   */
/* Compact a generation that is stored on the disk (in place).       */
/* Compact is done asynchronously and the callback function notifies */
/* the user when the operation has been completed.                   */
/* If the operation failed the callback function provides a reason   */
/* code and a description of the problem (e.g., disk error)          */
/*                                                                   */
/* @param genId         The generation Id                            */
/* @param pBufferParams Containing the bitMap of live blocks         */
/* @param callback      Callback function supplied with the data     */
/* @param pContext      Context to associate with the callback       */
/* @param threadNumber  Thread number                                */
/*                                                                   */
/* @return A return code, StoreRC_OK=success                  */
/*********************************************************************/
int ism_storeDisk_compactGeneration(ismStore_DiskTaskParams_t *pDiskTaskParams) ;

/*********************************************************************/
/* Compact store generation data in memory                           */
/*                                                                   */
/* Compact in memory generation into newly allocated memory          */
/* Compact is done synchronously.                                    */
/*                                                                   */
/* @param genData       The in memory generation                     */
/* @param pBufferParams Providing the bitMap on input and containing */
/*                      the allocated compacted version on output    */
/*                                                                   */
/* @return A return code, StoreRC_OK=success                  */
/*********************************************************************/
int ism_storeDisk_compactGenerationData(void *genData,
                                        ismStore_DiskBufferParams_t *pBufferParams);

/*********************************************************************/
/* Expand store generation compacted data in memory                  */
/*                                                                   */
/* Expand in memory compacted generation data into a preallocated    */
/* memory. Expand is done synchronously.                             */
/*                                                                   */
/* @param genData       The in memory compacted generation data      */
/* @param pBufferParams Providing the preallocated memory on input   */
/*                      and containing the expanded length on output */
/*                                                                   */
/* @return A return code, StoreRC_OK=success                  */
/*********************************************************************/
int ism_storeDisk_expandGenerationData(void *genData,
                                       ismStore_DiskBufferParams_t *pBufferParams);

/*********************************************************************/
/* Delete store generation data from the disk                        */
/*                                                                   */
/* Delete a generation that is stored on the disk.                   */
/* Delete is done asynchronously and the callback function  notifies */
/* the user when the operation has been completed.                   */
/* If the operation failed the callback function provides a reason   */
/* code and a description of the problem (e.g., disk error)          */
/*                                                                   */
/* @param genId         The generation Id                            */
/* @param callback      Callback function supplied with the data     */
/* @param pContext      Context to associate with the callback       */
/* @param threadNumber  Thread number                                */
/*                                                                   */
/* @return A return code, StoreRC_OK=success                  */
/*********************************************************************/
int ism_storeDisk_deleteGeneration(ismStore_DiskTaskParams_t *pDiskTaskParams) ;

/*********************************************************************/
/* Delete orphan generation files from the disk                      */
/*                                                                   */
/* Delete any generation that is stored on the disk and is not       */
/* present in the management as an existant generation.              */
/*                                                                   */
/* @return A return code, StoreRC_OK=success                  */
/*********************************************************************/
int ism_storeDisk_deleteDeadFiles(ismStore_DiskTaskParams_t *pDiskTaskParams) ;

/*********************************************************************/
/* Get the statistics of the store disk                              */
/*                                                                   */
/* @param pDiskStats    The store disk statistics                    */
/*                                                                   */
/* @return A return code, StoreRC_OK=success                  */
/*********************************************************************/
int ism_storeDisk_getStatistics(ismStore_DiskStatistics_t *pDiskStats);


int ism_storeDisk_removeCompactTasks(void);
int ism_storeDisk_compactTasksCount(int priority);

/*********************************************************************/
/* Get file size on disk                                             */
/*                                                                   */
/* @param path     The path to the file                              */
/* @param file     The file name                                     */
/* @param fileSize The returned size in bytes                        */
/*                                                                   */
/* @return A return code, StoreRC_OK=success                  */
/*********************************************************************/
int ism_storeDisk_getFileSize(const char *path, const char *file, uint64_t *fileSize);


/*********************************************************************/
/* Write a region of memory to the disk                              */
/*                                                                   */
/* Write is done asynchronously and the callback function notifies   */
/* the user when the operation has been completed.                   */
/* If the operation failed the callback function provides a reason   */
/* code and a description of the problem (e.g., out of disk space)   */
/*                                                                   */
/* @param path          The path to the file                         */
/* @param file          The file name                                */
/* @param pBufferParams The disk buffer parameters                   */
/* @param callback      Callback function supplied with the data     */
/* @param pContext      Context to associate with the callback       */
/* @param threadNumber  Thread number                                */
/*                                                                   */
/* @return A return code, StoreRC_OK=success                  */
/*********************************************************************/
int ism_storeDisk_writeFile(ismStore_DiskTaskParams_t *pDiskTaskParams) ;

/*********************************************************************/
/* Read a file from the disk to a memory region                      */
/*                                                                   */
/* Read is done asynchronously and the callback function notifies    */
/* the user when the operation has been completed.                   */
/* If the operation failed the callback function provides a reason   */
/* code and a description of the problem (e.g., disk error)          */
/*                                                                   */
/* @param path          The path to the file                         */
/* @param file          The file name                                */
/* @param pBufferParams The disk buffer parameters                   */
/* @param callback      Callback function supplied with the data     */
/* @param pContext      Context to associate with the callback       */
/* @param threadNumber  Thread number                                */
/*                                                                   */
/* @return A return code, StoreRC_OK=success                  */
/*********************************************************************/
int ism_storeDisk_readFile(ismStore_DiskTaskParams_t *pDiskTaskParams) ;

int ism_storeDisk_initDir(const char *path, ismStoe_DirInfo *di);
int ism_store_isTmpName(const char *x);

#ifdef __cplusplus
}
#endif

#endif /* __ISM_STORE_DISKUTILS_DEFINED */

/*********************************************************************/
/* End of storeDiskUtils.h                                           */
/*********************************************************************/
