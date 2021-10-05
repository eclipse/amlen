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
/* Module Name: storeMemoryHA.h                                      */
/*                                                                   */
/* Description: Store memory HA component header file                */
/*                                                                   */
/*********************************************************************/
#ifndef __ISM_STORE_MEMORYHA_DEFINED
#define __ISM_STORE_MEMORYHA_DEFINED

#define SHORT_SIZE   2
#define INT_SIZE     4
#define LONG_SIZE    8

#define ismSTORE_getChar(ptr,val)  {val=ptr[0];ptr++;}
#define ismSTORE_getShort(ptr,val) {uint16_t netval;memcpy(&netval,ptr,SHORT_SIZE);val=netval;ptr+=SHORT_SIZE;}
#define ismSTORE_getInt(ptr,val)   {uint32_t netval;memcpy(&netval,ptr,INT_SIZE);val=netval;ptr+=INT_SIZE;}
#define ismSTORE_getLong(ptr,val)  {uint64_t netval;memcpy(&netval,ptr,LONG_SIZE);val=netval;ptr+=LONG_SIZE;}

#define ismSTORE_putChar(ptr,val)  {ptr[0]=val;ptr++;}
#define ismSTORE_putShort(ptr,val) {uint16_t netval;netval=val;memcpy(ptr,&netval,SHORT_SIZE);ptr+=SHORT_SIZE;}
#define ismSTORE_putInt(ptr,val)   {uint32_t netval;netval=val;memcpy(ptr,&netval,INT_SIZE);ptr+=INT_SIZE;}
#define ismSTORE_putLong(ptr,val)  {uint64_t netval;netval=val;memcpy(ptr,&netval,LONG_SIZE);ptr+=LONG_SIZE;}

#define ismSTORE_HA_SYNC_THREAD_NAME "haSyncCh"
#define ismSTORE_HA_CHID_MIN_INTERNAL 10000  // Minimum internal channel ID
#define ismSTORE_HA_CHID_SYNC         10001  // Channel ID used for node synchronization
#define ismSTORE_HA_CHID_ADMIN        10002  // Channel ID used for Admin messages

#if 0
#define ismSTORE_HASSTANDBY          (ismStore_memGlobal.HAInfo.State == ismSTORE_HA_STATE_PRIMARY && ismStore_memGlobal.HAInfo.SyncNodesCount > 1)
#else
#define ismSTORE_HASSTANDBY          (ismStore_memGlobal.HAInfo.HasStandby)
#endif

#define ismSTORE_HA_SYNC_FLUSH       104857600 // 100 MB in bytes
#define ismSTORE_HA_SYNC_MAX_TIME 600000000000 // 10 minutes in nano seconds
#define ismSTORE_HA_SYNC_DIFF        104857600 // 100 MB in bytes

typedef enum
{
   StoreHAMsg_StoreTran       = 1,    /* Store transaction               */
   StoreHAMsg_UpdateActiveOid = 2,    /* Update Minimum Active OrderId   */
   StoreHAMsg_AssignRsrvPool  = 3,    /* Assign Mgmt Reserved Pool       */
   StoreHAMsg_CreateGen       = 4,    /* Create new generation           */
   StoreHAMsg_ActivateGen     = 5,    /* Activate generation             */
   StoreHAMsg_WriteGen        = 6,    /* Write generation to the disk    */
   StoreHAMsg_DeleteGen       = 7,    /* Delete generation from the disk */
   StoreHAMsg_CloseChannel    = 8,    /* Close channel gracefully        */
   StoreHAMsg_Shutdown        = 9,    /* Graceful shutdown               */
   StoreHAMsg_Ack             = 10,   /* Acknowledgment                  */
   StoreHAMsg_CompactGen      = 11,   /* Compact generation file         */

   StoreHAMsg_SyncList        = 30,   /* Sync generation list message    */
   StoreHAMsg_SyncListRes     = 31,   /* Sync generation list response   */
   StoreHAMsg_SyncDiskGen     = 32,   /* Sync generation file message    */
   StoreHAMsg_SyncMemGen      = 33,   /* Sync memory generation message  */
   StoreHAMsg_SyncComplete    = 34,   /* Sync complete message           */
   StoreHAMsg_SyncError       = 35,   /* Sync error message              */

   StoreHAMsg_Admin           = 50,   /* Admin message                   */
   StoreHAMsg_AdminFile       = 51    /* Admin file transfer             */
} ismStore_memHAMsgType_t;

typedef enum
{
   StoreHASyncGen_Empty       = 0,
   StoreHASyncGen_Proposed    = 1,
   StoreHASyncGen_Requested   = 2,
   StoreHASyncGen_Reading     = 3,
   StoreHASyncGen_Available   = 4,
   StoreHASyncGen_Sending     = 5,
   StoreHASyncGen_Sent        = 6,
   StoreHASyncGen_Error       = 7
} ismStore_memHASyncGenState_t;

/*********************************************************************/
/* Store High-Availability Job                                       */
/*                                                                   */
/* Contains information about job to be process by the store         */
/* High-Availability sync thread                                     */
/*********************************************************************/
typedef enum
{
   StoreHAJob_SyncList        = 1,
   StoreHAJob_SyncDiskGen     = 2,
   StoreHAJob_SyncMemGen      = 3,
   StoreHAJob_SyncComplete    = 4,
   StoreHAJob_SyncAbort       = 5
} ismStore_memHAJobType_t;

typedef struct ismStore_memHAJob_t
{
   ismStore_memHAJobType_t          JobType;
   ismStore_GenId_t                 GenId;
   struct ismStore_memHAJob_t      *pNextJob;
} ismStore_memHAJob_t;

#define  isLastFrag(x) ((x)&1)
#define setLastFrag(x) ((x)|=1)
#define clrLastFrag(x) ((x)&=(~1))

#define  isFlowCtrlAck(x) ((x)&2)
#define setFlowCtrlAck(x) ((x)|=2)
#define clrFlowCtrlAck(x) ((x)&=(~2))

#define  isNoAck(x) ((x)&4)
#define setNoAck(x) ((x)|=4)
#define clrNoAck(x) ((x)&=(~4))

typedef struct ismStore_memHAFragment_t
{
   ismStore_memHAMsgType_t          MsgType;
   uint32_t                         FragSqn;
   uint64_t                         MsgSqn;
   uint64_t                         DataLength;
   char                            *pData;
   char                            *pArg;
   struct ismStore_memHAFragment_t *pNext;
   uint8_t                          flags;
   uint8_t                          freeMe;
} ismStore_memHAFragment_t;

typedef struct ismStore_memHAAck_t
{
   uint64_t                         AckSqn;
   uint32_t                         FragSqn;
   ismStore_memHAMsgType_t          SrcMsgType;
   int32_t                          ReturnCode;
   uint32_t                         DataLength;
   uint8_t                          fFlowCtrlAck;
   char                            *pData;
} ismStore_memHAAck_t;

typedef struct ismStore_memHAChannel_t
{
   int32_t                          ChannelId;
   uint32_t                         FragSqn;
   uint32_t                         TxQDepth;
   uint32_t                         UnAckedFrags;
   uint8_t                          AckingPolicy;
   uint8_t                          fFlowControl;
   uint64_t                         MsgSqn;
   uint64_t                         FragsCount;
   uint64_t                         AckSqn;
   void                            *hChannel;
   ismStore_memHAFragment_t        *pFrag;
   ismStore_memHAFragment_t        *pFragTail;
} ismStore_memHAChannel_t;

typedef struct ismStore_memHAInfo_t
{
   ismStore_HAView_t                View;
   ismHA_View_t                     AdminView;
   uint8_t                          StartupMode;
   uint8_t                          AckingPolicy;
   uint8_t                          RoleValidation;
   uint8_t                          State;
   uint8_t                          AdminFileState;
   uint8_t                          fAdminTx;
   uint8_t                          MutexInit;
   pthread_mutex_t                  Mutex;
   pthread_cond_t                   ViewCond;
   pthread_cond_t                   SyncCond;

   uint8_t                          SyncState;   /* 0x0=None, 0x1=Start, 0x2=DiskGen1, 0x4=DiskGen2, 0x8=MemGen, 0x40=Admin, 0x80=Error */
   uint8_t                          fSyncLocked;
   uint8_t                          SyncNodesCount;
   uint8_t                          HasStandby;  /* (ismStore_memGlobal.HAInfo.State == ismSTORE_HA_STATE_PRIMARY && ismStore_memGlobal.HAInfo.SyncNodesCount > 1) */
   uint32_t                         SyncExpGensCount;
   uint32_t                         SyncSentGensCount;
   uint32_t                         SyncRC;
   uint64_t                         SyncMaxMemSizeBytes;
   uint64_t                         SyncCurMemSizeBytes;
   uint64_t                         SyncSentBytes;
   ism_time_t                       SyncTime[5]; /* 0=Start, 1=Admin, 2=DiskGen2, 3=Lock, 4=Complete */

   char                            *pAdminResBuff;

   ismStore_memHAChannel_t         *pIntChannel;
   ismStore_memHAChannel_t         *pSyncChannel;
   ismStore_memHAChannel_t         *pAdminChannel;
   uint8_t                          fAdminChannelBusy;

   uint16_t                         LPChIndex;
   uint16_t                         HPChIndex;

   ism_threadh_t                    ThreadId;
   pthread_mutex_t                  ThreadMutex;
   pthread_cond_t                   ThreadCond;
   uint8_t                          ThreadMutexInit;
   uint8_t                          fThreadGoOn;
   uint8_t                          fThreadUp;
   ismStore_memHAJob_t             *pThreadFirstJob;
   ismStore_memHAJob_t             *pThreadLastJob;
} ismStore_memHAInfo_t;

#define ismSTORE_HA_STATE_CLOSED         0x0
#define ismSTORE_HA_STATE_INIT           0x1
#define ismSTORE_HA_STATE_UNSYNC         0x2
#define ismSTORE_HA_STATE_STANDBY        0x3
#define ismSTORE_HA_STATE_PRIMARY        0x4
#define ismSTORE_HA_STATE_TERMINATING    0x5
#define ismSTORE_HA_STATE_ERROR          0x6

/*********************************************************************/
/* Internal function prototypes                                      */
/*********************************************************************/
int ism_store_memHAInit(void);

int ism_store_memHAStart(void);

int ism_store_memHATerm(void);

int ism_store_memHACreateChannel(
                   int32_t channelId,
                   uint8_t flags,
                   ismStore_memHAChannel_t **pHAChannel);

int ism_store_memHACloseChannel(
                   ismStore_memHAChannel_t *pHAChanne,
                   uint8_t fGracefully);

int ism_store_memHASendST(
                   ismStore_memHAChannel_t *pHAChannel,
                   ismStore_Handle_t hStoreTran);

int ism_store_memHASendActiveOid(
                   ismStore_memHAChannel_t *pHAChannel,
                   ismStore_Handle_t ownerHandle,
                   uint64_t minActiveOrderId);

int ism_store_memHASendGenMsg(
                   ismStore_memHAChannel_t *pHAChannel,
                   ismStore_GenId_t genId,
                   uint8_t genIndex,
                   uint8_t bitMapIndex,
                   ismStore_memHAMsgType_t msgType);

int ism_store_memHAReceiveAck(
                   ismStore_memHAChannel_t *pHAChannel,
                   ismStore_memHAAck_t *pAck,
                   uint8_t fNonBlocking);

int ism_store_memHASyncStart(void);

int ism_store_memHASyncWaitView(void);

int ism_store_memHASyncWaitDisk(void);

int8_t ism_store_memHAGetSyncCompPct(void);

#endif /* __ISM_STORE_MEMORYHA_DEFINED */

/*********************************************************************/
/* End of storeMemoryHA.h                                            */
/*********************************************************************/
