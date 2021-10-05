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
/* Module Name: Persist.h                                            */
/*                                                                   */
/* Description: Shm disk persistance header file                     */
/*                                                                   */
/*********************************************************************/
#ifndef __ISM_STORE_SHMPERSIST_DEFINED
#define __ISM_STORE_SHMPERSIST_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#define TRACE_COMP Store

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
#include "storeDiskUtils.h"
#include "storeMemory.h"

typedef struct ismStore_PersistParameters_t
{
   size_t                        TransferBlockSize;      /* Preferred read/write block size              */
   size_t                        DiskBlockSize;          /* Disk preferred block size (output parameter) */
   size_t                        PersistFileSize;        /* Max size for logged store transactions       */
   size_t                        StreamBuffSize;         /* Per stream buff size for STs                 */
   uint32_t                      PersistCbHwm;
   uint8_t                       PersistThreadPolicy;    /* Persist Thread Policy                        */
   uint8_t                       PersistAsyncThreads;    /* Persist Thread Policy                        */
   uint8_t                       PersistHaTxThreads;     /* Persist Thread Policy                        */
   uint8_t                       PersistAsyncOrdered;    /* Persist Thread Policy                        */
   uint8_t                       EnableHA;
   uint8_t                       RecoverFromV12;
   ismStore_AsyncCBStatsMode_t   AsyncCBStatsMode;       /* Do we track how many CBs we perform          */
   char                          RootPath[PATH_MAX + 1]; /* Root directory for stored persistent files   */
} ismStore_PersistParameters_t;

typedef enum
{
   PERSIST_STATE_UNINITIALIZED,
   PERSIST_STATE_INITIALIZED,
   PERSIST_STATE_STARTED
}  ismStore_PersistState_t ; 

int ism_storePersist_checkStopToken(void);
int ism_storePersist_coldStart(void);
int ism_storePersist_completeST(ismStore_memStream_t *pStream);
int ism_storePersist_openStream(ismStore_memStream_t *pStream);
int ism_storePersist_closeStream(ismStore_memStream_t *pStream);
int ism_storePersist_prepareClean(void);
int ism_storePersist_createCP(int needCP);
int ism_storePersist_doRecovery(void);
int ism_storePersist_havePState(void);
int ism_storePersist_init(ismStore_PersistParameters_t *pPersistParams);
int ism_storePersist_onGenClosed(int genIndex);
int ism_storePersist_onGenCreate(int genIndex);
int ism_storePersist_onGenWrite(int genIndex, int writeMsg);
int ism_storePersist_start(void);
int ism_storePersist_stop(void);
int ism_storePersist_term(int enforcCP);
int ism_storePersist_writeGenST(int block,ismStore_GenId_t genId,uint8_t genIndex,ismStore_memHAMsgType_t msgType);
int ism_storePersist_writeST(ismStore_memStream_t *pStream,int block);
int ism_storePersist_writeActiveOid(ismStore_Handle_t ownerHandle, uint64_t minActiveOrderId);
int ism_storePersist_emptyDir(const char *path);
int ism_storePersist_stopCB(void);
int ism_storePersist_flushQ(int wait4ack, int milli2wait);
int ism_storePersist_wait4lock(void);
int ism_storePersist_wrLock(void);
int ism_storePersist_wrUnlock(void);
int ism_storePersist_getStoreSize(char *RootPath, uint64_t *pSize);
int ism_storePersist_getState(void);
double ism_storePersist_getTimeStamp(void);
int ism_storePersist_getAsyncCBStats(uint32_t *pTotalReadyCBs, uint32_t *pTotalWaitingCBs,
                                     uint32_t *pNumThreads,
                                     ismStore_AsyncThreadCBStats_t *pCBThreadStats);

#ifdef __cplusplus
}
#endif

#endif
