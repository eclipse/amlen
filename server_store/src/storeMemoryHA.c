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
/* Module Name: storeMemoryHA.c                                      */
/*                                                                   */
/* Description: HA Memory module of Store Component of ISM           */
/*                                                                   */
/*********************************************************************/
#define TRACE_COMP Store
#include <store.h>            /* Store external header file             */
#include <ha.h>               /* High-Availability external header file */

#include "storeHighAvailability.h"
#include "storeInternal.h"
#include "storeMemoryHA.h"
#include "storeMemory.h"
#include "storeRecovery.h"
#include "storeDiskUtils.h"
#include "storeShmPersist.h"

extern ismStore_global_t    ismStore_global;
extern ismStore_memGlobal_t ismStore_memGlobal;

static pthread_mutex_t ismStore_HAAdminMutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct ismStore_memHADiskWriteCtxt_t
{
   /*--- The first fields in this structure MUST be the same as   ---*/
   /*--- the fields of ismStore_memDiskWriteCtxt_t                ---*/
   ismStore_memGeneration_t *pGen;
   char                     *pCompData;
   uint64_t                  CompDataSizeBytes;
   /*-------------- END OF ismStore_memDiskWriteCtxt_t --------------*/

   int32_t                   ViewId;
   ismStore_memHAChannel_t  *pHAChannel;
   void                     *pArg;
   ismStore_memHAAck_t       Ack;
} ismStore_memHADiskWriteCtxt_t;

/*********************************************************************/
/* Static functions definitions                                      */
/*********************************************************************/
static int  ism_store_memHAViewChanged(
                   ismStore_HAView_t *pView,
                   void *pContext);

static int  ism_store_memHAChannelCreated(
                   int32_t channelId,
                   uint8_t flags,
                   void *hChannel,
                   ismStore_HAChParameters_t *pChParams,
                   void *pContext);

static int  ism_store_memHAChannelClosed(
                   void *hChannel,
                   void *pChContext);

static int  ism_store_memHAMsgReceived(
                   void *hChannel,
                   char *pData,
                   uint32_t dataLength,
                   void *pChContext);

static int  ism_store_memHAIntMsgReceived(
                   void *hChannel,
                   char *pData,
                   uint32_t dataLength,
                   void *pChContext);

static int  ism_store_memHAParseMsg(
                   ismStore_memHAChannel_t *pHAChannel);

static int  ism_store_memHAParseGenMsg(
                   ismStore_memHAChannel_t *pHAChannel);

static int  ism_store_memHAEnsureBufferAllocation(
                   ismStore_memHAChannel_t *pHAChannel,
                   char **pBuffer,
                   uint32_t *bufferLength,
                   char **pPos,
                   uint32_t requiredLength,
                   ismStore_memHAMsgType_t msgType,
                   uint32_t *opcount);

static int  ism_store_memHASendAck(
                   ismStore_memHAChannel_t *pHAChannel,
                   ismStore_memHAAck_t *pAck);

static void ism_store_memHADiskWriteComplete(
                   ismStore_GenId_t genId,
                   int32_t retcode,
                   ismStore_DiskGenInfo_t *pDiskGenInfo,
                   void *pContext);

static void ism_store_memHADiskDeleteComplete(
                   ismStore_GenId_t genId,
                   int32_t retcode,
                   ismStore_DiskGenInfo_t *pDiskGenInfo,
                   void *pContext);

static void ism_store_memHADiskDeleteDeadComplete(
                   ismStore_GenId_t genId,
                   int32_t retcode,
                   ismStore_DiskGenInfo_t *pDiskGenInfo,
                   void *pContext);

static void ism_store_memHAAdminDiskReadComplete(
                   ismStore_GenId_t genId,
                   int32_t retcode,
                   ismStore_DiskGenInfo_t *pDiskGenInfo,
                   void *pContext);

static void ism_store_memHAAdminDiskWriteComplete(
                   ismStore_GenId_t genId,
                   int32_t retcode,
                   ismStore_DiskGenInfo_t *pDiskGenInfo,
                   void *pContext);

static void ism_store_memHAAddJob(
                   ismStore_memHAJob_t *pJob);

static int ism_store_memHASyncComplete(void);

static int ism_store_memHASyncLock(void);

static int ism_store_memHASyncSendList(void);

static int ism_store_memHASyncReadDiskGen(void);

static int ism_store_memHASyncSendDiskGen(
                   ismStore_GenId_t genId);

static int ism_store_memHASyncSendMemGen(void);

static void ism_store_memHASyncDiskReadComplete(
                   ismStore_GenId_t genId,
                   int32_t retcode,
                   ismStore_DiskGenInfo_t *pDiskGenInfo,
                   void *pContext);

static void ism_store_memHASyncDiskWriteComplete(
                   ismStore_GenId_t genId,
                   int32_t retcode,
                   ismStore_DiskGenInfo_t *pDiskGenInfo,
                   void *pContext);

static void *ism_store_memHASyncThread(
                   void *arg,
                   void *pContext,
                   int value);

static void ism_store_memHASetHasStandby(void);

int ism_store_memHAInit(void)
{
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   ismStore_HAParameters_t haParams;
   int rc = StoreRC_OK;

   TRACE(9, "Entry: %s. State %u\n", __FUNCTION__, pHAInfo->State);
   if (pHAInfo->State != ismSTORE_HA_STATE_CLOSED)
   {
      TRACE(1, "Failed to initialize the HA component, because the component is already initialized. State %d\n", pHAInfo->State);
      rc = StoreRC_SystemError;
      goto exit;
   }

   if (pthread_mutex_init(&pHAInfo->Mutex, NULL))
   {
      TRACE(1, "Failed to initialize the HA mutex (Mutex)\n");
      rc = StoreRC_SystemError;
      goto exit;
   }
   pHAInfo->MutexInit++;

   if (pthread_cond_init(&pHAInfo->ViewCond, NULL))
   {
      TRACE(1, "Failed to initialize the HA cond (ViewCond)\n");
      rc = StoreRC_SystemError;
      goto exit;
   }
   pHAInfo->MutexInit++;

   if (ism_common_cond_init_monotonic(&pHAInfo->SyncCond))
   {
      TRACE(1, "Failed to initialize the HA cond (SyncCond)\n");
      rc = StoreRC_SystemError;
      goto exit;
   }
   pHAInfo->MutexInit++;

   if ((pHAInfo->pAdminResBuff = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,105),ismHA_MAX_ADMIN_RESPONSE_LENGTH)) == NULL)
   {
      TRACE(1, "Failed to allocate memory (%u) for the Admin response buffer\n", ismHA_MAX_ADMIN_RESPONSE_LENGTH);
      rc = StoreRC_SystemError;
      goto exit;
   }

   memset(&haParams, '\0', sizeof(haParams));
   haParams.ViewChanged = ism_store_memHAViewChanged;
   haParams.ChannelCreated = ism_store_memHAChannelCreated;
   haParams.pContext = NULL;
   if ((rc = ism_storeHA_init(&haParams)) != StoreRC_OK)
   {
      TRACE(1, "Failed to initialize the HA component. error code %d\n", rc);
      goto exit;
   }
   pthread_mutex_lock(&ismStore_HAAdminMutex);
   pHAInfo->State = ismSTORE_HA_STATE_INIT;
   pthread_mutex_unlock(&ismStore_HAAdminMutex);
   TRACE(5, "The HA component has been initialized successfully\n");

exit:
   if (rc != StoreRC_OK)
   {
      if (pHAInfo->MutexInit > 0)
      {
         pthread_mutex_destroy(&pHAInfo->Mutex);
         if (pHAInfo->MutexInit > 1)
         {
            pthread_cond_destroy(&pHAInfo->ViewCond);
            if (pHAInfo->MutexInit > 2)
            {
               pthread_cond_destroy(&pHAInfo->SyncCond);
            }
         }
         pHAInfo->MutexInit = 0;
      }
      ismSTORE_FREE(pHAInfo->pAdminResBuff);
   }

   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
   return rc;
}

int ism_store_memHAStart(void)
{
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   int rc = StoreRC_OK;

   TRACE(9, "Entry: %s. State %u\n", __FUNCTION__, pHAInfo->State);
   if (pHAInfo->State != ismSTORE_HA_STATE_INIT)
   {
      TRACE(1, "Failed to start the HA component, because the component is not initialized. State %d\n", pHAInfo->State);
      rc = StoreRC_SystemError;
      goto exit;
   }

   // Initialize the HASync thread variables
   if (pthread_mutex_init(&pHAInfo->ThreadMutex, NULL))
   {
      TRACE(1, "Failed to initialize HA mutex (ThreadMutex)\n");
      rc = StoreRC_SystemError;
      goto exit;
   }
   pHAInfo->ThreadMutexInit = 1;

   if (pthread_cond_init(&pHAInfo->ThreadCond, NULL))
   {
      TRACE(1, "Failed to initialize HA cond (ThreadCond)\n");
      rc = StoreRC_SystemError;
      goto exit;
   }
   pHAInfo->ThreadMutexInit = 2;

   if ((rc = ism_storeHA_start()) != StoreRC_OK)
   {
      TRACE(1, "Failed to start the HA component. error code %d\n", rc);
      goto exit;
   }

   // Wait for the first ViewChanged callback
   pthread_mutex_lock(&pHAInfo->Mutex);
   while (pHAInfo->State == ismSTORE_HA_STATE_INIT)   /* BEAM suppression: infinite loop */
   {
      pthread_cond_wait(&pHAInfo->ViewCond, &pHAInfo->Mutex);
   }

   if (pHAInfo->State == ismSTORE_HA_STATE_ERROR)
   {
      TRACE(1, "Failed to start the HA component. Role %u, ActiveNodesCount %u\n",
            pHAInfo->View.NewRole, pHAInfo->View.ActiveNodesCount);
      pthread_mutex_unlock(&pHAInfo->Mutex);
      rc = StoreRC_SystemError;
      goto exit;
   }
   else
   {
      TRACE(5, "The HA component has been started successfully. State %d, Role %u, ActiveNodesCount %u, SyncNodesCount %u\n",
            pHAInfo->State, pHAInfo->View.NewRole, pHAInfo->View.ActiveNodesCount, pHAInfo->SyncNodesCount);
   }
   pthread_mutex_unlock(&pHAInfo->Mutex);

exit:
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
   return rc;
}

int ism_store_memHATerm(void)
{
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   int rc = StoreRC_OK;

   TRACE(9, "Entry: %s. State %u, fAdminChannelBusy %u\n", __FUNCTION__, pHAInfo->State, pHAInfo->fAdminChannelBusy);
   if (pHAInfo->State == ismSTORE_HA_STATE_CLOSED)
   {
      TRACE(1, "Failed to terminate the HA component, because the component is not initialized\n");
      rc = StoreRC_SystemError;
      goto exit;
   }

   pthread_mutex_lock(&ismStore_HAAdminMutex);
   pHAInfo->State = ismSTORE_HA_STATE_TERMINATING;
   // Wait until the AdminChannel is free
   while (pHAInfo->fAdminChannelBusy)   /* BEAM suppression: infinite loop */
   {
      pthread_mutex_unlock(&ismStore_HAAdminMutex);
      ism_common_sleep(1000);
      pthread_mutex_lock(&ismStore_HAAdminMutex);
   }
   pthread_mutex_unlock(&ismStore_HAAdminMutex);

   pthread_mutex_lock(&pHAInfo->Mutex);
   pHAInfo->State = ismSTORE_HA_STATE_TERMINATING;
   pHAInfo->fThreadGoOn = 0;
   pthread_cond_signal(&pHAInfo->ViewCond);
   pthread_cond_signal(&pHAInfo->SyncCond);

   // Wait until the HASync thread terminates
   while (pHAInfo->fThreadUp)   /* BEAM suppression: infinite loop */
   {
      pthread_cond_signal(&pHAInfo->ThreadCond);
      pthread_mutex_unlock(&pHAInfo->Mutex);
      ism_common_sleep(1000);
      pthread_mutex_lock(&pHAInfo->Mutex);
   }
   pthread_mutex_unlock(&pHAInfo->Mutex);

   // Close the Admin channel (if exists)
   if (pHAInfo->fAdminTx && pHAInfo->pAdminChannel)
   {
      pthread_mutex_lock(&ismStore_HAAdminMutex);
      ism_store_memHACloseChannel(pHAInfo->pAdminChannel, 1);
      pHAInfo->pAdminChannel = NULL;
      pHAInfo->fAdminTx = 0;  // Mark the Admin channel as receiver
      pthread_mutex_unlock(&ismStore_HAAdminMutex);
   }

   if ((rc = ism_storeHA_term()) != StoreRC_OK)
   {
      TRACE(1, "Failed to terminate the HA component. error code %d\n", rc);
      goto exit;
   }

   if (pHAInfo->ThreadMutexInit > 0)
   {
      pthread_mutex_destroy(&pHAInfo->ThreadMutex);
      if (pHAInfo->ThreadMutexInit > 1)
      {
         pthread_cond_destroy(&pHAInfo->ThreadCond);
      }
      pHAInfo->ThreadMutexInit = 0;
   }

   ismSTORE_FREE(pHAInfo->pAdminResBuff);
   pthread_mutex_destroy(&pHAInfo->Mutex);
   pthread_cond_destroy(&pHAInfo->ViewCond);
   pthread_cond_destroy(&pHAInfo->SyncCond);
   pHAInfo->MutexInit = 0;
   pHAInfo->State = ismSTORE_HA_STATE_CLOSED;
   TRACE(5, "The HA component has been terminated successfully\n");

exit:
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
   return rc;
}

int ism_store_memHACreateChannel(int32_t channelId, uint8_t flags, ismStore_memHAChannel_t **pHAChannel)
{
   ismStore_memHAChannel_t *pChannel;
   const ismStore_HAConfig_t *pHAConfig;
   int rc;

   if ((pChannel = (ismStore_memHAChannel_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,106),sizeof(ismStore_memHAChannel_t))) == NULL)
   {
      TRACE(1, "Failed to allocate memory for the HA channel (ChannelId %d, Flags 0x%x)\n", channelId, flags);
      return StoreRC_SystemError;
   }

   memset(pChannel, '\0', sizeof(ismStore_memHAChannel_t));
   pHAConfig = ism_storeHA_getConfig();
   if (pHAConfig->FlowControl == 1 && (flags & 0x4))
   {
      pChannel->fFlowControl = 1;
   }

   if ((rc = ism_storeHA_createChannel(channelId, flags, &pChannel->hChannel)) == StoreRC_HA_ViewChanged)
   {
      TRACE(4, "Failed to create an HA channel (ChannelId %d, Flags 0x%x), because the Standby node has failed\n",
            channelId, flags);
      ismSTORE_FREE(pChannel);
      return StoreRC_OK;
   }

   if (rc != StoreRC_OK)
   {
      TRACE(1, "Failed to create an HA channel (ChannelId %d, Flags 0x%x). error code %d\n",
            channelId, flags, rc);
      ismSTORE_FREE(pChannel);
      return rc;
   }

   if (pChannel->fFlowControl)
   {
      if ((rc = ism_storeHA_getChannelTxQDepth(pChannel->hChannel, &pChannel->TxQDepth)) != StoreRC_OK)
      {
         TRACE(1, "Failed to create an HA channel (ChannelId %d, Flags 0x%x) because the ism_storeHA_getChannelTxQDepth failed. error code %d\n",
               channelId, flags, rc);
         ismSTORE_FREE(pChannel);
         return rc;
      }

      if (pChannel->TxQDepth <= 1)
      {
         // TxQDepth = 1 indicates that the replication protocol is not RDMA RC.
         pChannel->fFlowControl = 0;
      }
   }

   if ((pChannel->ChannelId = channelId) == 0)
   {
      ismStore_memGlobal.HAInfo.pIntChannel = pChannel;
   }
   *pHAChannel = pChannel;

   TRACE(5, "An HA channel (ChannelId %d, Flags 0x%x, TxQDepth %u) has been created successfully\n", channelId, flags, pChannel->TxQDepth);

   return StoreRC_OK;
}

static void ism_store_memHAFreeFrags(ismStore_memHAChannel_t *pHAChannel)
{
   ismStore_memHAFragment_t *pFrag;

   while (pHAChannel->pFrag)
   {
      pFrag = pHAChannel->pFrag;
      pHAChannel->pFrag = pFrag->pNext;
      if (pFrag->freeMe)
      {
         ismSTORE_FREE(pFrag);
      }
   }
   pHAChannel->pFrag = pHAChannel->pFragTail = NULL ; 
}

int ism_store_memHACloseChannel(ismStore_memHAChannel_t *pHAChannel, uint8_t fGracefully)
{
   ismStore_memHAMsgType_t msgType = StoreHAMsg_CloseChannel;
   ismStore_memHAAck_t ack;
   char *pBuffer=NULL, *pPos;
   uint32_t opcount, bufferLength;
   int rc;

   if (!pHAChannel || !pHAChannel->hChannel)
   {
      return StoreRC_SystemError;
   }

   if (fGracefully)
   {
      // Mark the channel as close pending
      if ((rc = ism_storeHA_closeChannel(pHAChannel->hChannel, 1)) != StoreRC_OK)
      {
       TRACE(1, "Failed to mark the HA channel (ChannelId %d) as close pending. error code %d\n", pHAChannel->ChannelId, rc);
       return rc;
      }

      // Send a message to the remote node
      if ((rc = ism_store_memHAEnsureBufferAllocation(pHAChannel,
                                                      &pBuffer,
                                                      &bufferLength,
                                                      &pPos,
                                                      64,
                                                      msgType,
                                                      &opcount)) != StoreRC_OK)
      {
         return rc;
      }

      // Send the last fragment
      // requiredLength = 0 means that this is the last fragment.
      if ((rc = ism_store_memHAEnsureBufferAllocation(pHAChannel, &pBuffer, &bufferLength, &pPos, 0, msgType, &opcount)) == StoreRC_OK)
      {
         TRACE(9, "An HA message (ChannelId %d, MsgType %u, MsgSqn %lu, LastFrag %u, AckSqn %lu) has been sent\n",
               pHAChannel->ChannelId, msgType, pHAChannel->MsgSqn - 1, pHAChannel->FragSqn, pHAChannel->AckSqn);

         // Wait for connection broke from the remote node
         do
         {
            memset(&ack, '\0', sizeof(ack));
            rc = ism_store_memHAReceiveAck(pHAChannel, &ack, 0);
         } while (rc == StoreRC_OK);
      }
   }

   // Close the channel and free the resources
   if ((rc = ism_storeHA_closeChannel(pHAChannel->hChannel, 0)) != StoreRC_OK)
   {
      TRACE(1, "Failed to close the HA channel (ChannelId %d). error code %d\n", pHAChannel->ChannelId, rc);
      return rc;
   }

   ism_store_memHAFreeFrags(pHAChannel);

   TRACE(5, "The HA channel (ChannelId %d) has been closed successfully\n", pHAChannel->ChannelId);
   ismSTORE_FREE(pHAChannel);

   return StoreRC_OK;
}

int ism_store_memHASendST(ismStore_memHAChannel_t *pHAChannel, ismStore_Handle_t hStoreTran)
{
   ismStore_memDescriptor_t *pDescriptor, *pOpDescriptor;
   ismStore_memStoreTransaction_t *pTran;
   ismStore_memStoreOperation_t *pOp;
   ismStore_memSplitItem_t *pSplit;
   ismStore_memReferenceChunk_t *pRefChunk;
   ismStore_memRefStateChunk_t *pRefStateChunk;
   ismStore_memStateChunk_t *pStateChunk;
   ismStore_memState_t *pStateObject;
   ismStore_Handle_t handle, chunkHandle, offset;
   ismStore_GenId_t genId;
   ismStore_memHAMsgType_t msgType = StoreHAMsg_StoreTran;
   char *pBuffer=NULL, *pPos, *pTmp;
   int loop, rc, i;
   uint32_t opcount, bufferLength;

   pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(hStoreTran);
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
               handle = pOp->CreateReference.Handle;

               if ((rc = ism_store_memHAEnsureBufferAllocation(pHAChannel,
                                                               &pBuffer,
                                                               &bufferLength,
                                                               &pPos,
                                                               64,
                                                               msgType,
                                                               &opcount)) != StoreRC_OK)
               {
                  return rc;
               }

               genId = ismSTORE_EXTRACT_GENID(handle);
               offset = ismSTORE_EXTRACT_OFFSET(handle);
               chunkHandle = ismSTORE_BUILD_HANDLE(genId, ismSTORE_ROUND(offset, ismStore_memGlobal.pGensMap[genId]->GranulesMap[0].GranuleSizeBytes));
               pOpDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(chunkHandle);
               pRefChunk = (ismStore_memReferenceChunk_t *)(pOpDescriptor + 1);

               ismSTORE_putShort(pPos, pOp->OperationType);
               ismSTORE_putInt(pPos, 4 * LONG_SIZE + INT_SIZE + 1);
               ismSTORE_putLong(pPos, handle);
               ismSTORE_putLong(pPos, pRefChunk->OwnerHandle);
               ismSTORE_putLong(pPos, pRefChunk->BaseOrderId);
               ismSTORE_putLong(pPos, pOp->CreateReference.RefHandle);
               ismSTORE_putInt(pPos, pOp->CreateReference.Value);
               ismSTORE_putChar(pPos, pOp->CreateReference.State);
               opcount++;
               break;

            case Operation_UpdateReference:
               handle = pOp->UpdateReference.Handle;

               if ((rc = ism_store_memHAEnsureBufferAllocation(pHAChannel,
                                                               &pBuffer,
                                                               &bufferLength,
                                                               &pPos,
                                                               64,
                                                               msgType,
                                                               &opcount)) != StoreRC_OK)
               {
                  return rc;
               }

               ismSTORE_putShort(pPos, pOp->OperationType);
               ismSTORE_putInt(pPos, LONG_SIZE + 1);
               ismSTORE_putLong(pPos, handle);
               ismSTORE_putChar(pPos, pOp->UpdateReference.State);
               opcount++;
               break;

            case Operation_DeleteReference:
               handle = pOp->DeleteReference.Handle;
               if ((rc = ism_store_memHAEnsureBufferAllocation(pHAChannel,
                                                               &pBuffer,
                                                               &bufferLength,
                                                               &pPos,
                                                               64,
                                                               msgType,
                                                               &opcount)) != StoreRC_OK)
               {
                  return rc;
               }

               ismSTORE_putShort(pPos, pOp->OperationType);
               ismSTORE_putInt(pPos, LONG_SIZE);
               ismSTORE_putLong(pPos, handle);
               opcount++;
               break;

            case Operation_CreateRecord:
               handle = pOp->CreateRecord.Handle;
               loop=0;
               while (handle != ismSTORE_NULL_HANDLE)
               {
                  pOpDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(handle);
                  if ((rc = ism_store_memHAEnsureBufferAllocation(pHAChannel,
                                                                  &pBuffer,
                                                                  &bufferLength,
                                                                  &pPos,
                                                                  pOpDescriptor->DataLength + sizeof(*pOpDescriptor),
                                                                  msgType,
                                                                  &opcount)) != StoreRC_OK)
                  {
                     return rc;
                  }
                  ismSTORE_putShort(pPos, pOp->OperationType);
                  ismSTORE_putInt(pPos, LONG_SIZE + sizeof(*pOpDescriptor) + pOpDescriptor->DataLength);
                  ismSTORE_putLong(pPos, handle);
                  memcpy(pPos, pOpDescriptor, sizeof(*pOpDescriptor) + pOpDescriptor->DataLength);

                  // Replace the temporary DataType with the exact DataType of the record
                  if (pOpDescriptor->DataType == ismSTORE_DATATYPE_NEWLY_HATCHED)
                  {
                     pTmp = pPos + offsetof(ismStore_memDescriptor_t, DataType);
                     ismSTORE_putShort(pTmp, pOp->CreateRecord.DataType);
                  }
                  pPos += sizeof(*pOpDescriptor) + pOpDescriptor->DataLength;
                  opcount++;
                  handle = pOpDescriptor->NextHandle;
                  if (handle == ismSTORE_NULL_HANDLE && loop == 0)
                  {
                     handle = pOp->CreateRecord.LDHandle;
                     loop++;
                  }
               }

               break;

            case Operation_DeleteRecord:
               handle = pOp->DeleteRecord.Handle;
               while (ismSTORE_EXTRACT_OFFSET(handle) > 0)
               {
                  genId = ismSTORE_EXTRACT_GENID(handle);
                  if (genId == pTran->GenId || genId == ismSTORE_MGMT_GEN_ID)
                  {
                     pOpDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(handle);
                     if ((rc = ism_store_memHAEnsureBufferAllocation(pHAChannel,
                                                                     &pBuffer,
                                                                     &bufferLength,
                                                                     &pPos,
                                                                     64,
                                                                     msgType,
                                                                     &opcount)) != StoreRC_OK)
                     {
                        return rc;
                     }

                     ismSTORE_putShort(pPos, pOp->OperationType);
                     ismSTORE_putInt(pPos, LONG_SIZE);
                     ismSTORE_putLong(pPos, handle);
                     opcount++;

                     if (ismSTORE_IS_SPLITITEM(pOpDescriptor->DataType))
                     {
                        pSplit = (ismStore_memSplitItem_t *)(pOpDescriptor + 1);
                        handle = pSplit->hLargeData;
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
               handle = pOp->UpdateRecord.Handle;
               pOpDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(handle);
               if ((rc = ism_store_memHAEnsureBufferAllocation(pHAChannel,
                                                               &pBuffer,
                                                               &bufferLength,
                                                               &pPos,
                                                               64,
                                                               msgType,
                                                               &opcount)) != StoreRC_OK)
               {
                  return rc;
               }
               ismSTORE_putShort(pPos, pOp->OperationType);
               if (pOp->OperationType == Operation_UpdateRecord)
               {
                  ismSTORE_putInt(pPos, 3 * LONG_SIZE);
                  ismSTORE_putLong(pPos, handle);
                  ismSTORE_putLong(pPos, pOp->UpdateRecord.Attribute);
                  ismSTORE_putLong(pPos, pOp->UpdateRecord.State);
               }
               else
               {
                  ismSTORE_putInt(pPos, 2 * LONG_SIZE);
                  ismSTORE_putLong(pPos, handle);
                  if (pOp->OperationType == Operation_UpdateRecordAttr)
                  {
                     ismSTORE_putLong(pPos, pOp->UpdateRecord.Attribute);
                  }
                  else
                  {
                     ismSTORE_putLong(pPos, pOp->UpdateRecord.State);
                  }
               }
               opcount++;
               break;

            case Operation_UpdateRefState:
               handle = pOp->UpdateReference.Handle;
               genId = ismSTORE_EXTRACT_GENID(handle);
               offset = ismSTORE_EXTRACT_OFFSET(handle);

               if ((rc = ism_store_memHAEnsureBufferAllocation(pHAChannel,
                                                               &pBuffer,
                                                               &bufferLength,
                                                               &pPos,
                                                               64,
                                                               msgType,
                                                               &opcount)) != StoreRC_OK)
               {
                  return rc;
               }

               chunkHandle = ismSTORE_BUILD_HANDLE(genId, ismSTORE_ROUND(offset, ismStore_memGlobal.MgmtSmallGranuleSizeBytes));
               pOpDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(chunkHandle);
               pRefStateChunk = (ismStore_memRefStateChunk_t *)(pOpDescriptor + 1);

               ismSTORE_putShort(pPos, pOp->OperationType);
               ismSTORE_putInt(pPos, 3 * LONG_SIZE + 1);
               ismSTORE_putLong(pPos, handle);
               ismSTORE_putLong(pPos, pRefStateChunk->OwnerHandle);
               ismSTORE_putLong(pPos, pRefStateChunk->BaseOrderId);
               ismSTORE_putChar(pPos, pOp->UpdateReference.State);
               opcount++;
               break;

            case Operation_CreateState:
               handle = pOp->CreateState.Handle;
               genId = ismSTORE_EXTRACT_GENID(handle);
               offset = ismSTORE_EXTRACT_OFFSET(handle);
               chunkHandle = ismSTORE_BUILD_HANDLE(genId, ismSTORE_ROUND(offset, ismStore_memGlobal.MgmtGranuleSizeBytes));
               pOpDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(chunkHandle);
               pStateChunk = (ismStore_memStateChunk_t *)(pOpDescriptor + 1);
               pStateObject = (ismStore_memState_t *)ism_store_memMapHandleToAddress(handle);
               if ((rc = ism_store_memHAEnsureBufferAllocation(pHAChannel,
                                                               &pBuffer,
                                                               &bufferLength,
                                                               &pPos,
                                                               64,
                                                               msgType,
                                                               &opcount)) != StoreRC_OK)
               {
                  return rc;
               }

               ismSTORE_putShort(pPos, pOp->OperationType);
               ismSTORE_putInt(pPos, 2 * LONG_SIZE + INT_SIZE + 1);
               ismSTORE_putLong(pPos, handle);
               ismSTORE_putLong(pPos, pStateChunk->OwnerHandle);
               ismSTORE_putInt(pPos, pStateObject->Value);
               ismSTORE_putChar(pPos, ismSTORE_STATEFLAG_VALID);
               opcount++;
               break;

            case Operation_DeleteState:
               handle = pOp->DeleteState.Handle;
               if ((rc = ism_store_memHAEnsureBufferAllocation(pHAChannel,
                                                               &pBuffer,
                                                               &bufferLength,
                                                               &pPos,
                                                               64,
                                                               msgType,
                                                               &opcount)) != StoreRC_OK)
               {
                  return rc;
               }

               ismSTORE_putShort(pPos, pOp->OperationType);
               ismSTORE_putInt(pPos, LONG_SIZE);
               ismSTORE_putLong(pPos, handle);
               opcount++;
               break;

            case Operation_UpdateActiveOid:
               if ((rc = ism_store_memHAEnsureBufferAllocation(pHAChannel,
                                                               &pBuffer,
                                                               &bufferLength,
                                                               &pPos,
                                                               64,
                                                               msgType,
                                                               &opcount)) != StoreRC_OK)
               {
                  return rc;
               }

               ismSTORE_putShort(pPos, pOp->OperationType);
               ismSTORE_putInt(pPos, 2 * LONG_SIZE);
               ismSTORE_putLong(pPos, pOp->UpdateActiveOid.OwnerHandle + sizeof(ismStore_memDescriptor_t) + offsetof(ismStore_memSplitItem_t, MinActiveOrderId));
               ismSTORE_putLong(pPos, pOp->UpdateActiveOid.OrderId);
               opcount++;
               break;

            default:
               TRACE(1, "Failed to encode a store-transaction operation for HA, because the operation type %d is not valid, ChannelId %d\n", pOp->OperationType, pHAChannel->ChannelId);
               return StoreRC_SystemError;
         }
      }

      if (pDescriptor->NextHandle == ismSTORE_NULL_HANDLE)
      {
         break;
      }
      pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(pDescriptor->NextHandle);
   }

   // Send the last fragment
   // requiredLength = 0 means that this is the last fragment.
   // pBuffer = Null means that the store-transaction is empty
   if (!pBuffer)
   {
      TRACE(8, "The store-transaction is empty. No HA message was sent to the Standby node. ChannelId %d, MsgSqn %lu\n", pHAChannel->ChannelId, pHAChannel->MsgSqn);
      rc = StoreRC_HA_MsgUnsent;
   }
   else if ((rc = ism_store_memHAEnsureBufferAllocation(pHAChannel, &pBuffer, &bufferLength, &pPos, 0, msgType, &opcount)) == StoreRC_OK)
   {
      TRACE(9, "An HA ST message (ChannelId %d, MsgType %u, MsgSqn %lu, FragsCount %u, AckSqn %lu) has been sent\n",
            pHAChannel->ChannelId, msgType, pHAChannel->MsgSqn - 1, pHAChannel->FragSqn, pHAChannel->AckSqn);
   }

   return rc;
}

int ism_store_memHASendActiveOid(ismStore_memHAChannel_t *pHAChannel, ismStore_Handle_t ownerHandle, uint64_t minActiveOrderId)
{
   ismStore_memHAMsgType_t msgType = StoreHAMsg_UpdateActiveOid;
   char *pBuffer=NULL, *pPos;
   int rc;
   uint32_t opcount, bufferLength;

   if ((rc = ism_store_memHAEnsureBufferAllocation(pHAChannel,
                                                   &pBuffer,
                                                   &bufferLength,
                                                   &pPos,
                                                   64,
                                                   msgType,
                                                   &opcount)) != StoreRC_OK)
   {
      return rc;
   }

   ismSTORE_putShort(pPos, Operation_UpdateActiveOid);
   ismSTORE_putInt(pPos, 2 * LONG_SIZE);
   ismSTORE_putLong(pPos, ownerHandle + sizeof(ismStore_memDescriptor_t) + offsetof(ismStore_memSplitItem_t, MinActiveOrderId));
   ismSTORE_putLong(pPos, minActiveOrderId);
   opcount++;

   // Send the last fragment
   // requiredLength = 0 means that this is the last fragment.
   if ((rc = ism_store_memHAEnsureBufferAllocation(pHAChannel, &pBuffer, &bufferLength, &pPos, 0, msgType, &opcount)) == StoreRC_OK)
   {
      TRACE(9, "An HA ActiveOrderId message (ChannelId %d, MsgType %u, MsgSqn %lu, LastFrag %u, AckSqn %lu) has been sent. OwnerHandle %lu, MiniumActiveOrderId %lu\n",
            pHAChannel->ChannelId, msgType, pHAChannel->MsgSqn - 1, pHAChannel->FragSqn, pHAChannel->AckSqn, ownerHandle, minActiveOrderId);
   }

   return rc;
}

int ism_store_memHASendGenMsg(
                   ismStore_memHAChannel_t *pHAChannel,
                   ismStore_GenId_t genId,
                   uint8_t genIndex,
                   uint8_t bitMapIndex,
                   ismStore_memHAMsgType_t msgType)
{
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memGenHeader_t  *pGenHeader, *pGenHeaderPrev;
   ismStore_memGenMap_t *pGenMap;
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_memGenIdChunk_t *pGenIdChunk;
   ismStore_Handle_t handle;
   char *pBuffer=NULL, *pPos;
   int rc;
   uint8_t genIndexPrev, poolId;
   uint32_t opcount, bufferLength, fragLength, dataLength, offset;
   uint64_t *pBitMap;

   if ((rc = ism_store_memHAEnsureBufferAllocation(pHAChannel,
                           &pBuffer,
                           &bufferLength,
                           &pPos,
                           64 + sizeof(ismStore_memGenHeader_t),
                           msgType,
                           &opcount)) != StoreRC_OK)
   {
      return rc;
   }

   pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;
   pGenHeader = (genId == ismSTORE_MGMT_GEN_ID ? (ismStore_memGenHeader_t *)pMgmtHeader : (ismStore_memGenHeader_t *)ismStore_memGlobal.InMemGens[genIndex].pBaseAddress);

   // Generation information
   ismSTORE_putShort(pPos, Operation_Null);
   ismSTORE_putInt(pPos, SHORT_SIZE + 1);
   ismSTORE_putShort(pPos, genId);
   ismSTORE_putChar(pPos, genIndex);
   opcount++;

   // Generation Token
   if (msgType == StoreHAMsg_WriteGen ||
       msgType == StoreHAMsg_Shutdown)
   {
      ismSTORE_putShort(pPos, Operation_CreateRecord);
      ismSTORE_putInt(pPos, LONG_SIZE + sizeof(ismStore_memGenToken_t));
      handle = ismSTORE_BUILD_HANDLE(genId, offsetof(ismStore_memGenHeader_t, GenToken));
      ismSTORE_putLong(pPos, handle);
      memcpy(pPos, (char *)&pGenHeader->GenToken, sizeof(ismStore_memGenToken_t));
      pPos += sizeof(ismStore_memGenToken_t);
      opcount++;
   }

   // Generation header
   if (msgType == StoreHAMsg_CreateGen)
   {
      ismSTORE_putShort(pPos, Operation_CreateRecord);
      ismSTORE_putInt(pPos, LONG_SIZE + offsetof(ismStore_memGenHeader_t, Reserved));
      handle = ismSTORE_BUILD_HANDLE(genId, 0);
      ismSTORE_putLong(pPos, handle);
      memcpy(pPos, ismStore_memGlobal.InMemGens[genIndex].pBaseAddress, offsetof(ismStore_memGenHeader_t, Reserved));
      pPos += offsetof(ismStore_memGenHeader_t, Reserved);
      opcount++;
   }

   // Generation state
   if (msgType == StoreHAMsg_ActivateGen)
   {
      ismSTORE_putShort(pPos, Operation_CreateRecord);
      ismSTORE_putInt(pPos, LONG_SIZE + 1);
      handle = ismSTORE_BUILD_HANDLE(genId, offsetof(ismStore_memGenHeader_t, State));
      ismSTORE_putLong(pPos, handle);
      ismSTORE_putChar(pPos, pGenHeader->State);
      opcount++;

      // See whether we have to change the state of the previous generation to ismSTORE_GEN_STATE_CLOSE_PENDING
      genIndexPrev = (genIndex + ismStore_memGlobal.InMemGensCount - 1) % ismStore_memGlobal.InMemGensCount;
      pGenHeaderPrev = (ismStore_memGenHeader_t *)ismStore_memGlobal.InMemGens[genIndexPrev].pBaseAddress;
      if (pGenHeaderPrev->State == ismSTORE_GEN_STATE_CLOSE_PENDING)
      {
         ismSTORE_putShort(pPos, Operation_CreateRecord);
         ismSTORE_putInt(pPos, LONG_SIZE + 1);
         handle = ismSTORE_BUILD_HANDLE(pGenHeaderPrev->GenId, offsetof(ismStore_memGenHeader_t, State));
         ismSTORE_putLong(pPos, handle);
         ismSTORE_putChar(pPos, ismSTORE_GEN_STATE_CLOSE_PENDING);
         opcount++;
         TRACE(7, "The state of the previous generation (GenId %u, GenIndex %u) is being changed to ismSTORE_GEN_STATE_CLOSE_PENDING on the Standby node\n",
               pGenHeaderPrev->GenId, genIndexPrev);
      }
   }

   // Management generation header
   if (msgType == StoreHAMsg_CreateGen ||
       msgType == StoreHAMsg_DeleteGen ||
       msgType == StoreHAMsg_ActivateGen ||
       msgType == StoreHAMsg_AssignRsrvPool)
   {
      ismSTORE_putShort(pPos, Operation_CreateRecord);
      ismSTORE_putInt(pPos, LONG_SIZE + offsetof(ismStore_memMgmtHeader_t, Reserved2));
      ismSTORE_putLong(pPos, ismSTORE_BUILD_HANDLE(ismSTORE_MGMT_GEN_ID, 0));
      memcpy(pPos, pMgmtHeader, offsetof(ismStore_memMgmtHeader_t, Reserved2));
      pPos += offsetof(ismStore_memMgmtHeader_t, Reserved2);
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
         if ((rc = ism_store_memHAEnsureBufferAllocation(pHAChannel,
                                                         &pBuffer,
                                                         &bufferLength,
                                                         &pPos,
                                                         dataLength,
                                                         msgType,
                                                         &opcount)) != StoreRC_OK)
         {
            return rc;
         }
         ismSTORE_putShort(pPos, Operation_CreateRecord);
         ismSTORE_putInt(pPos, LONG_SIZE + dataLength);
         ismSTORE_putLong(pPos, handle);
         memcpy(pPos, pDescriptor, dataLength);
         pPos += dataLength;
         opcount++;
         handle = pDescriptor->NextHandle;
      }
   }

   // GenMap
   if (msgType == StoreHAMsg_WriteGen ||
       msgType == StoreHAMsg_CompactGen)
   {
      pGenMap = ismStore_memGlobal.pGensMap[genId];
      ismSTORE_putShort(pPos, Operation_AllocateGranulesMap);
      ismSTORE_putInt(pPos, LONG_SIZE + 1);
      ismSTORE_putLong(pPos, pGenMap->PredictedSizeBytes);
      ismSTORE_putChar(pPos, pGenMap->GranulesMapCount); // Number of pools
      opcount++;

      for (poolId=0; poolId < pGenMap->GranulesMapCount; poolId++)
      {
         if ((rc = ism_store_memHAEnsureBufferAllocation(pHAChannel,
                                                         &pBuffer,
                                                         &bufferLength,
                                                         &pPos,
                                                         64,
                                                         msgType,
                                                         &opcount)) != StoreRC_OK)
         {
            return rc;
         }

         pBitMap = pGenMap->GranulesMap[poolId].pBitMap[bitMapIndex];
         dataLength = pGenMap->GranulesMap[poolId].BitMapSize * sizeof(uint64_t);
         offset = 0;
         ismSTORE_putShort(pPos, Operation_CreateGranulesMap);
         ismSTORE_putInt(pPos, 2 + INT_SIZE);
         ismSTORE_putChar(pPos, poolId);      // Pool Id
         ismSTORE_putChar(pPos, bitMapIndex); // BitMap index
         ismSTORE_putInt(pPos, dataLength);   // Map length
         opcount++;

         while (1)
         {
            fragLength = bufferLength - ((uint32_t)(pPos - pBuffer) + SHORT_SIZE + INT_SIZE + 2 + INT_SIZE);
            if (dataLength < fragLength) {
               fragLength = dataLength;
            }

            ismSTORE_putShort(pPos, Operation_SetGranulesMap);
            ismSTORE_putInt(pPos, fragLength + 2 + INT_SIZE);
            ismSTORE_putChar(pPos, poolId);      // Pool Id
            ismSTORE_putChar(pPos, bitMapIndex); // BitMap index
            ismSTORE_putInt(pPos, offset);       // Offset

            if (fragLength > 0)
            {
               memcpy(pPos, (char *)pBitMap + offset, fragLength);
               pPos += fragLength;
            }
            opcount++;

            dataLength -= fragLength;
            offset += fragLength;

            if (dataLength == 0) { break; }

            if ((rc = ism_store_memHAEnsureBufferAllocation(pHAChannel,
                                                            &pBuffer,
                                                            &bufferLength,
                                                            &pPos,
                                                            dataLength,
                                                            msgType,
                                                            &opcount)) != StoreRC_OK)
            {
               return rc;
            }
         }
      }
   }

   // Send the last fragment
   // requiredLength = 0 means that this is the last fragment.
   if ((rc = ism_store_memHAEnsureBufferAllocation(pHAChannel, &pBuffer, &bufferLength, &pPos, 0, msgType, &opcount)) == StoreRC_OK)
   {
      TRACE(7, "An HA Gen message (ChannelId %d, MsgType %u, MsgSqn %lu, LastFrag %u, AckSqn %lu) has been sent. GenId %u, GenIndex %u\n",
            pHAChannel->ChannelId, msgType, pHAChannel->MsgSqn - 1, pHAChannel->FragSqn, pHAChannel->AckSqn, genId, genIndex);
   }

   return rc;
}

static int ism_store_memHASendAck(ismStore_memHAChannel_t *pHAChannel, ismStore_memHAAck_t *pAck)
{
   ismStore_memHAMsgType_t msgType = StoreHAMsg_Ack;
   char *pBuffer=NULL, *pPos=NULL;
   int rc;
   uint32_t opcount, bufferLength;

   if ((rc = ism_store_memHAEnsureBufferAllocation(pHAChannel,
                                                   &pBuffer,
                                                   &bufferLength,
                                                   &pPos,
                                                   64 + pAck->DataLength,
                                                   msgType,
                                                   &opcount)) != StoreRC_OK)
   {
      return rc;
   }

   // Make sure that there is enough space in the allocated send buffer for the ACK data
   if (pAck->DataLength > 0 && bufferLength < pAck->DataLength + 64)
   {
      TRACE(1, "The HA ACK data (DataLength %u) has been truncated, because the allocated send buffer is too small (BufferLength %u). ReturnCode %d\n",
            pAck->DataLength, bufferLength, pAck->ReturnCode);
      pAck->DataLength = bufferLength - 64;
   }

   ismSTORE_putShort(pPos, Operation_Null);
   ismSTORE_putInt(pPos, LONG_SIZE + SHORT_SIZE + 2 * INT_SIZE);
   ismSTORE_putLong(pPos, pAck->AckSqn);
   ismSTORE_putInt(pPos, pAck->FragSqn);
   ismSTORE_putShort(pPos, pAck->SrcMsgType);
   ismSTORE_putInt(pPos, pAck->ReturnCode);
   opcount++;

   if (pAck->DataLength > 0)
   {
      ismSTORE_putShort(pPos, Operation_Null);
      ismSTORE_putInt(pPos, pAck->DataLength);
      memcpy(pPos, pAck->pData, pAck->DataLength);
      pPos += pAck->DataLength;
      opcount++;
   }

   // Send the last fragment
   // requiredLength = 0 means that this is the last fragment.
   if ((rc = ism_store_memHAEnsureBufferAllocation(pHAChannel, &pBuffer, &bufferLength, &pPos, 0, msgType, &opcount)) == StoreRC_OK)
   {
      TRACE(9, "An HA ACK message (ChannelId %d, MsgType %u, MsgSqn 0, LastFrag %u) has been sent. AckSqn %lu, FragSqn %u, SrcMsgType %u, ReturnCode %d, DataLength %u\n",
            pHAChannel->ChannelId, StoreHAMsg_Ack, pHAChannel->FragSqn, pAck->AckSqn, pAck->FragSqn, pAck->SrcMsgType, pAck->ReturnCode, pAck->DataLength);
   }

   return rc;
}

int ism_store_memHAReceiveAck(ismStore_memHAChannel_t *pHAChannel, ismStore_memHAAck_t *pAck, uint8_t fNonBlocking)
{
   ismStore_memHAMsgType_t msgType;
   ismStore_memOperationType_t opType;
   char *pData, *ptr, *pBase;
   int rc;
   uint64_t msgSqn, expectedSqn = pAck->AckSqn;
   uint32_t fragSqn, fragLength, dataLength, resLength, buffLength, opcount, expectedFragSqn = pAck->FragSqn;
   uint8_t flags, lastFrag;

   // Sanity check - the expected sqn must be less than the next message sqn
   if (!fNonBlocking &&
       ((!pAck->fFlowCtrlAck && expectedSqn >= pHAChannel->MsgSqn) || (pAck->fFlowCtrlAck && expectedFragSqn >= pHAChannel->FragSqn)))
   {
      TRACE(1, "The expected HA Ack (AckSqn %lu, FragSqn %u, fFlowCtrlAck %u) is not valid. ChannelId %u, MsgSqn %lu, AckSqn %lu, FragSqn %u\n",
            expectedSqn, expectedFragSqn, pAck->fFlowCtrlAck, pHAChannel->ChannelId, pHAChannel->MsgSqn, pHAChannel->AckSqn, pHAChannel->FragSqn);
      return StoreRC_SystemError;
   }

   buffLength = pAck->DataLength;
   pAck->DataLength = 0;

   if ((rc = ism_storeHA_receiveMessage(pHAChannel->hChannel, &pData, &dataLength, fNonBlocking)) != StoreRC_OK)
   {
      if (rc == StoreRC_HA_ConnectionBroke)
      {
         TRACE(9, "Failed to receive an HA ACK message (ChannelId %d). error code %d\n", pHAChannel->ChannelId, rc);
      }
      else if (rc != StoreRC_HA_WouldBlock)
      {
         TRACE(1, "Failed to receive a message ACK on the HA channel (ChannelId %d). error code %d\n", pHAChannel->ChannelId, rc);
      }
      return rc;
   }

   ptr = pData;
   ismSTORE_getInt(ptr, fragLength);          // Fragment length
   ismSTORE_getShort(ptr, msgType);           // Message type
   if (msgType != StoreHAMsg_Ack)
   {
      TRACE(1, "Failed to receive a message ACK on the HA channel (ChannelId %d), because the message type (%d) is not valid\n", pHAChannel->ChannelId, msgType);
      return StoreRC_SystemError;
   }

   ismSTORE_getLong(ptr, msgSqn);             // Message sequence
   ismSTORE_getInt(ptr, fragSqn);             // Fragment sequence
   ismSTORE_getChar(ptr, flags);              // Fragment flags
   lastFrag = isLastFrag(flags);              // IsLastFragment
   ptr += INT_SIZE;                           // Reserved
   ismSTORE_getInt(ptr, opcount);             // Operations count
   pBase = ptr;

   ismSTORE_getShort(ptr, opType);            // Operation type
   ismSTORE_getInt(ptr, dataLength);          // Data length

   if (opcount < 1 || opType != Operation_Null)
   {
      TRACE(1, "Failed to receive a message ACK on the HA channel (ChannelId %d), because the message header is not valid. opcount %d, opType %d, dataLength %u\n",
            pHAChannel->ChannelId, opcount, opType, dataLength);
      return StoreRC_SystemError;
   }

   ismSTORE_getLong(ptr, pAck->AckSqn);       // Ack sequence
   ismSTORE_getInt(ptr, pAck->FragSqn);       // Fragment sequence
   ismSTORE_getShort(ptr, pAck->SrcMsgType);  // Source message type
   ismSTORE_getInt(ptr, pAck->ReturnCode);    // Return code

   TRACE(9, "An HA fragment (ChannelId %d, FragLength %u, MsgType %u, MsgSqn %lu, FragSqn %u, IsLastFrag %d, OpCount %u) has been received. " \
         "OpType %d, AckSqn %lu, ExpectedSqn %lu, FragSqn %u, ExpectedFragSqn %u, SrcMsgType %u, ReturnCode %d, DataLength %u, UnAckedFrags %u\n",
         pHAChannel->ChannelId, fragLength, msgType, msgSqn, fragSqn, lastFrag, opcount, opType, pAck->AckSqn, expectedSqn,
         pAck->FragSqn, expectedFragSqn,  pAck->SrcMsgType, pAck->ReturnCode, pAck->DataLength, pHAChannel->UnAckedFrags);
   pHAChannel->UnAckedFrags = 0;

   ptr = pBase + (SHORT_SIZE + INT_SIZE + dataLength);
   if (opcount > 1)
   {
      if (buffLength == 0 || !pAck->pData)
      {
         TRACE(1, "Could not receive the ACK data (AckSqn %lu, SrcMsgType 0x%x, ReturnCode %d) on the HA channel (ChannelId %d), because a receive buffer was not provided (BufferLength %u, pBuffer %p)\n",
               pAck->AckSqn, pAck->SrcMsgType, pAck->ReturnCode, pHAChannel->ChannelId, buffLength, pAck->pData);
      }
      else
      {
         ismSTORE_getShort(ptr, opType);         // Operation type
         ismSTORE_getInt(ptr, dataLength);       // Data length
         if (dataLength == 0)
         {
            TRACE(1, "Failed to receive a message ACK (AckSqn %lu, SrcMsgType 0x%x, ReturnCode %d) on the HA channel (ChannelId %d), because the ACK data is not valid. DataLength %u\n",
                  pAck->AckSqn, pAck->SrcMsgType, pAck->ReturnCode, pHAChannel->ChannelId, dataLength);
            return StoreRC_SystemError;
         }

         if ((resLength = dataLength) > buffLength)
         {
            TRACE(1, "The message ACK data (AckSqn %lu, SrcMsgType 0x%x, ReturnCode %d, DataLength %u) on the HA channel (ChanneldId %d) has been truncated, because the receive buffer is too small (BufferLength %u)\n",
                  pAck->AckSqn, pAck->SrcMsgType, pAck->ReturnCode, dataLength, pHAChannel->ChannelId, buffLength);
            resLength = buffLength;
         }
         pAck->DataLength = resLength;
         if (resLength > 0)
         {
            memcpy(pAck->pData, ptr, resLength);
         }
         ptr += dataLength;
      }
   }

   if (pAck->AckSqn > pHAChannel->MsgSqn)
   {
      TRACE(1, "The HA ACK (AckSqn %lu, SrcMsgType %u, ReturnCode %d, DataLength %u) is not valid. ChannelId %d, FragLength %u, MsgType %u, MsgSqn %lu, FragSqn %u, IsLastFrag %d, OpCount %u, LastMsgSqn %lu\n",
            pAck->AckSqn, pAck->SrcMsgType, pAck->ReturnCode, pAck->DataLength, pHAChannel->ChannelId, fragLength, msgType, msgSqn, fragSqn, lastFrag, opcount, pHAChannel->MsgSqn);
   }

   if ((expectedSqn && pAck->AckSqn != expectedSqn) || (expectedFragSqn && pAck->FragSqn != expectedFragSqn))
   {
      TRACE(1, "The HA ACK (AckSqn %lu, FragSqn %u, SrcMsgType %u, ReturnCode %d, DataLength %u) is not as expected (AckSqn %lu, FragSqn %u). ChannelId %d, FragLength %u, MsgType %u, MsgSqn %lu, FragSqn %u, IsLastFrag %d, OpCount %u, LastMsgSqn %lu\n",
            pAck->AckSqn, pAck->FragSqn, pAck->SrcMsgType, pAck->ReturnCode, pAck->DataLength, expectedSqn, expectedFragSqn, pHAChannel->ChannelId, fragLength, msgType, msgSqn, fragSqn, lastFrag, opcount, pHAChannel->MsgSqn);
   }
   pHAChannel->AckSqn = pAck->AckSqn;

   if ((rc = ism_storeHA_returnBuffer(pHAChannel->hChannel, pData)) != StoreRC_OK)
   {
      TRACE(1, "Failed to return a buffer of HA received message (ChannelId %d, MsgType %u, MsgSqn %lu, AckSqn %lu, SrcMsgType %u, ReturnCode %d, DataLength %u). error code %d\n",
            pHAChannel->ChannelId, msgType, msgSqn, pAck->AckSqn, pAck->SrcMsgType, pAck->ReturnCode, pAck->DataLength, rc);
   }

   return StoreRC_OK;
}

static int ism_store_memHAEnsureBufferAllocation(ismStore_memHAChannel_t *pHAChannel,
                                                 char **pBuffer,
                                                 uint32_t *bufferLength,
                                                 char **pPos,
                                                 uint32_t requiredLength,
                                                 ismStore_memHAMsgType_t msgType,
                                                 uint32_t *opcount)
{
   ismStore_memHAAck_t ack;
   char *ptr;
   int rc;
   uint8_t flags = 0;
   uint32_t fragLength = (*pBuffer ? (*pPos - *pBuffer) : 0);

   // See whether there is no free space in the current buffer
   // requiredLenegth = 0 means that this is the last fragment.
   if (*pBuffer && (requiredLength == 0 || (fragLength + requiredLength + 64) > *bufferLength))
   {
      // Set the message fragment header
      ptr = *pBuffer;
      ismSTORE_putInt(ptr, fragLength - INT_SIZE);           // Fragment length
      ismSTORE_putShort(ptr, msgType);                       // Message type
      ismSTORE_putLong(ptr, pHAChannel->MsgSqn);             // Message sequence
      ismSTORE_putInt(ptr, pHAChannel->FragSqn);             // Fragment sequence

      pHAChannel->UnAckedFrags++;
      if (requiredLength == 0)
      {
         setLastFrag(flags);                                       // Last fragment
      }
      else if (pHAChannel->fFlowControl && pHAChannel->UnAckedFrags >= pHAChannel->TxQDepth)
      {
         setFlowCtrlAck(flags);                                       // FlowControl Ack is required
         memset(&ack, '\0', sizeof(ack));
         ack.AckSqn = pHAChannel->MsgSqn;
         ack.FragSqn = pHAChannel->FragSqn;
         ack.fFlowCtrlAck = 1;
      }

      ismSTORE_putChar(ptr, flags);                          // Flags
      ismSTORE_putInt(ptr, 0);                               // Reserved
      ismSTORE_putInt(ptr, *opcount);                        // Operations count
      if ((rc = ism_storeHA_sendMessage(pHAChannel->hChannel, *pBuffer, fragLength)) != StoreRC_OK)
      {
         if (rc == StoreRC_HA_ConnectionBroke)
         {
            TRACE(9, "Failed to send an HA message (ChannelId %d, MsgType %u, MsqSqn %lu, FragSqn %u, FragLength %u), because the connection broke. State %d\n",
                  pHAChannel->ChannelId, msgType, pHAChannel->MsgSqn, pHAChannel->FragSqn, fragLength, ismStore_memGlobal.HAInfo.State);
         }
         else
         {
            TRACE(1, "Failed to send a message on the HA channel (ChannelId %d, MsgType %u, MsqSqn %lu, FragSqn %u, FragLength %u, Flags 0x%x). error code %d\n",
                  pHAChannel->ChannelId, msgType, pHAChannel->MsgSqn, pHAChannel->FragSqn, fragLength, flags, rc);
         }
         return rc;
      }
      pHAChannel->FragsCount++;
      if (requiredLength == 0)
      {
         pHAChannel->MsgSqn++;
      }
      pHAChannel->FragSqn++;
      *pBuffer = NULL;

      // If a flow control is used, we have to wait for an Ack
      if (isFlowCtrlAck(flags))
      {
         if ((rc = ism_store_memHAReceiveAck(pHAChannel, &ack, 0)) != StoreRC_OK)
         {
            if (rc == StoreRC_HA_ConnectionBroke)
            {
               TRACE(9, "Failed to send a message on the HA channel (ChannelId %d, MsgType %u, MsqSqn %lu, FragSqn %u, FragLength %u, Flags 0x%x), because the connection broke. State %d\n",
                     pHAChannel->ChannelId, msgType, pHAChannel->MsgSqn, pHAChannel->FragSqn - 1, fragLength, flags, ismStore_memGlobal.HAInfo.State);
            }
            else
            {
               TRACE(1, "Failed to send a message on the HA channel (ChannelId %d, MsgType %u, MsqSqn %lu, FragSqn %u, FragLength %u, Flags 0x%x), due to flow control Ack failure (%d). error code %d\n",
                     pHAChannel->ChannelId, msgType, pHAChannel->MsgSqn, pHAChannel->FragSqn - 1, fragLength, flags, ack.ReturnCode, rc);
            }
            return rc;
         }
         TRACE(9, "A flow control Ack has been received on the HA channel (ChannelId %d, MsgType %u, MsqSqn %lu, FragSqn %u, FragLength %u, Flags 0x%x)\n",
               pHAChannel->ChannelId, msgType, pHAChannel->MsgSqn, pHAChannel->FragSqn - 1, fragLength, flags);
      }
   }
   else if (*pBuffer == NULL)
   {
      pHAChannel->FragSqn = 0;
   }

   // Allocate a new buffer
   if (*pBuffer == NULL && requiredLength > 0)
   {
      if ((rc = ism_storeHA_allocateBuffer(pHAChannel->hChannel, pBuffer, bufferLength)) != StoreRC_OK)
      {
         if (rc == StoreRC_HA_ConnectionBroke)
         {
            TRACE(9, "Failed to allocate a buffer for an HA message (ChannelId %d, MsgType %u, MsgSqn %lu, FragSqn %u). error code %d\n",
                  pHAChannel->ChannelId, msgType, pHAChannel->MsgSqn, pHAChannel->FragSqn, rc);
         }
         else
         {
            TRACE(1, "Failed to allocate a buffer for an HA message (ChannelId %d, MsgType %u, MsgSqn %lu, FragSqn %u). error code %d\n",
                  pHAChannel->ChannelId, msgType, pHAChannel->MsgSqn, pHAChannel->FragSqn, rc);
         }
         return rc;
      }

      if (*bufferLength < ismStore_memGlobal.GranuleSizeBytes || *bufferLength < ismStore_memGlobal.MgmtGranuleSizeBytes)
      {
         TRACE(1, "The allocated buffer (BufferLength %u) for an HA message (ChannelId %d, MsgType %u, MsgSqn %lu, FragSqn %u) is too small\n",
               *bufferLength, pHAChannel->ChannelId, msgType, pHAChannel->MsgSqn, pHAChannel->FragSqn);
         return StoreRC_SystemError;
      }
      *pPos = *pBuffer + (LONG_SIZE + 4 * INT_SIZE + SHORT_SIZE + 1);      // Skip on the message header
      *opcount = 0;
   }

   return StoreRC_OK;
}

static int ism_store_memHAViewChanged(ismStore_HAView_t *pView, void *pContext)
{
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   ismStore_memJob_t job;
   char nodeStr1[40], nodeStr2[40];
   int rc = StoreRC_OK;

   TRACE(9, "Entry: %s. ViewId %u\n", __FUNCTION__, pView->ViewId);

   ism_store_memB2H(nodeStr1, (uint8_t *)pView->ActiveNodeIds[0], sizeof(ismStore_HANodeID_t));
   if (pView->ActiveNodesCount > 1) {
      ism_store_memB2H(nodeStr2, (uint8_t *)pView->ActiveNodeIds[1], sizeof(ismStore_HANodeID_t));
   } else {
      nodeStr2[0] = 0;
   }
   TRACE(5, "HAView: The HA view has been changed. ViewId %u, NewRole %u, OldRole %u, ActiveNodesCount %u, ActiveNodeIds=[%s, %s], SyncState 0x%x, SyncMemSizeBytes %lu, State %d\n",
         pView->ViewId, pView->NewRole, pView->OldRole, pView->ActiveNodesCount, nodeStr1, nodeStr2, pHAInfo->SyncState, pHAInfo->SyncCurMemSizeBytes, pHAInfo->State);

   pthread_mutex_lock(&pHAInfo->Mutex);

   memset(&job, '\0', sizeof(job));
   job.JobType = StoreJob_HAViewChanged;
   memcpy(&pHAInfo->View, pView, sizeof(ismStore_HAView_t));

   switch (pView->NewRole)
   {
      case ISM_HA_ROLE_PRIMARY:
         if (pHAInfo->State == ismSTORE_HA_STATE_INIT)
         {
            TRACE(5, "HAView: The node has been selected to act as the Primary in the HA pair. ActiveNodesCount %u\n", pView->ActiveNodesCount);
            pHAInfo->SyncNodesCount = 1;  // Mark the primary as synchronized node
            pHAInfo->State = ismSTORE_HA_STATE_PRIMARY;
         }
         else if (pHAInfo->State ==  ismSTORE_HA_STATE_STANDBY)
         {
            TRACE(5, "HAView: The old Primary node has failed. This node has been selected to act as the new Primary in the HA pair\n");
            pHAInfo->SyncNodesCount = pView->ActiveNodesCount;
            pHAInfo->State = ismSTORE_HA_STATE_PRIMARY;
            job.JobType = StoreJob_HAStandby2Primary;
         }
         else if  (pHAInfo->State == ismSTORE_HA_STATE_PRIMARY)
         {
            if (pHAInfo->SyncNodesCount > pView->ActiveNodesCount)
            {
               TRACE(5, "HAView: The Standby node has left the HA pair\n");
               pHAInfo->SyncNodesCount = pView->ActiveNodesCount;
               job.JobType = StoreJob_HAStandbyLeft;
            }
            else if (pHAInfo->SyncNodesCount < pView->ActiveNodesCount)
            {
               TRACE(5, "HAView: A Standby node has joined the HA pair\n");
               job.JobType = StoreJob_HAStandbyJoined;
            }
            else if (pView->ActiveNodesCount == 1 && ismStore_memGlobal.State == ismSTORE_STATE_INIT)
            {
               TRACE(1, "HAView: The Standby node has failed during new node synchronization. ViewId %u, NewRole %u, OldRole %u, ActiveNodesCount %u, State %u\n",
                     pView->ViewId, pView->NewRole, pView->OldRole, pView->ActiveNodesCount, pHAInfo->State);
               pHAInfo->SyncNodesCount = 0;
               pHAInfo->View.NewRole = ISM_HA_ROLE_ERROR;
               pHAInfo->State = ismSTORE_HA_STATE_ERROR;
               rc = StoreRC_SystemError;
            }
         }
         else
         {
            TRACE(1, "HAView: The node could not act as the Primary in the HA pair, because it is not synchronized yet. ViewId %u, NewRole %u, OldRole %u, ActiveNodesCount %u, State %u\n",
                  pView->ViewId, pView->NewRole, pView->OldRole, pView->ActiveNodesCount, pHAInfo->State);
            pHAInfo->SyncNodesCount = 0;
            pHAInfo->View.NewRole = ISM_HA_ROLE_ERROR;
            pHAInfo->State = ismSTORE_HA_STATE_ERROR;
            rc = StoreRC_SystemError;
         }
         break;

      case ISM_HA_ROLE_STANDBY:
         if (pHAInfo->State == ismSTORE_HA_STATE_INIT)
         {
            TRACE(5, "HAView: The node has been selected to act as the Standby in the HA pair\n");
            pHAInfo->View.NewRole = ISM_HA_ROLE_UNSYNC;
            pHAInfo->State = ismSTORE_HA_STATE_UNSYNC;
         }
         else
         {
            TRACE(1, "HAView: The new role is not valid. ViewId %u, NewRole %u, OldRole %u, ActiveNodesCount %u, State %u\n",
                  pView->ViewId, pView->NewRole, pView->OldRole, pView->ActiveNodesCount, pHAInfo->State);
            pHAInfo->View.NewRole = ISM_HA_ROLE_ERROR;
            pHAInfo->State = ismSTORE_HA_STATE_ERROR;
            rc = StoreRC_SystemError;
         }
         break;

      case ISM_HA_ROLE_TERM:
         if (pHAInfo->State == ismSTORE_HA_STATE_STANDBY || pHAInfo->State == ismSTORE_HA_STATE_UNSYNC)
         {
            TRACE(5, "HAView: The HA pair has been terminated by the Primary node\n");
            pHAInfo->State = ismSTORE_HA_STATE_UNSYNC ; //ismSTORE_HA_STATE_TERMINATING;
            pthread_cond_signal(&pHAInfo->SyncCond);
         }
         else
         {
            TRACE(1, "HAView: The new role is not valid. ViewId %u, NewRole %u, OldRole %u, ActiveNodesCount %u, State %u\n",
                  pView->ViewId, pView->NewRole, pView->OldRole, pView->ActiveNodesCount, pHAInfo->State);
            pHAInfo->View.NewRole = ISM_HA_ROLE_ERROR;
            pHAInfo->State = ismSTORE_HA_STATE_ERROR;
            rc = StoreRC_SystemError;
         }
         break;

      default:
         TRACE(1, "HAView: The node failed due to an HA error. ViewId %u, NewRole %u, OldRole %u, ActiveNodesCount %u, ReasonCode %u, ReasonParam %s, State %u\n",
               pView->ViewId, pView->NewRole, pView->OldRole, pView->ActiveNodesCount, pView->ReasonCode, pView->pReasonParam, pHAInfo->State);

         pHAInfo->SyncNodesCount = 0;
         pHAInfo->State = ismSTORE_HA_STATE_ERROR;
   }

   job.HAView.ViewId = pHAInfo->View.ViewId;
   job.HAView.NewRole = pHAInfo->View.NewRole;
   job.HAView.OldRole = pHAInfo->View.OldRole;
   job.HAView.ActiveNodesCount = pHAInfo->View.ActiveNodesCount;

   ism_store_memHASetHasStandby() ; 

   TRACE(5, "HAView: NewRole %u, NewState %u, SyncNodesCount %u, SyncState 0x%x, JobType %u, rc %d\n",
         pView->NewRole, pHAInfo->State, pHAInfo->SyncNodesCount, pHAInfo->SyncState, job.JobType, rc);
   pthread_mutex_unlock(&pHAInfo->Mutex);

   if (pView->ActiveNodesCount < 2)
   {
      if (pHAInfo->SyncState > 0)
      {
         // Signal to the HASync thread to terminate (if it runs)
         // We don't wait for the thread's termination here, because we don't want to hold the HA thread
         pthread_mutex_lock(&pHAInfo->ThreadMutex);
         pHAInfo->fThreadGoOn = 0;
         pthread_cond_signal(&pHAInfo->ThreadCond);
         pthread_mutex_unlock(&pHAInfo->ThreadMutex);
      }

      // Close the Admin channel
      if (pHAInfo->fAdminTx && pHAInfo->pAdminChannel)
      {
         pthread_mutex_lock(&ismStore_HAAdminMutex);
         ism_store_memHACloseChannel(pHAInfo->pAdminChannel, 0);
         pHAInfo->pAdminChannel = NULL;
         pHAInfo->fAdminTx = 0;  // Mark the Admin channel as receiver
         pthread_mutex_unlock(&ismStore_HAAdminMutex);
      }
   }

   ism_store_memAddJob(&job);

   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

static int ism_store_memHAChannelCreated(int32_t channelId, uint8_t flags, void *hChannel, ismStore_HAChParameters_t *pChParams, void *pContext)
{
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   ismStore_memHAChannel_t *pHAChannel;

   TRACE(9, "Entry: %s. ChannelId %d\n", __FUNCTION__, channelId);

   if ((pHAChannel = (ismStore_memHAChannel_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,107),sizeof(ismStore_memHAChannel_t))) == NULL)
   {
      TRACE(1, "Failed to allocate memory for an HA channel (ChannelId %d, Flags 0x%x)\n", channelId, flags);
      return StoreRC_SystemError;
   }

   memset(pChParams, '\0', sizeof(ismStore_HAChParameters_t));
   memset(pHAChannel, '\0', sizeof(ismStore_memHAChannel_t));
   pHAChannel->ChannelId = channelId;
   pHAChannel->hChannel = hChannel;
   pHAChannel->AckingPolicy = 1;  // By default we send ACK after the processing of the message

   pthread_mutex_lock(&pHAInfo->Mutex);
   // Set the channel name based on the channel Id
   if (channelId == 0)
   {
      snprintf(pChParams->ChannelName, 16, "haSBStoreCh");
      pChParams->MessageReceived = ism_store_memHAMsgReceived;
      pChParams->fMultiSend = 1;
      pHAInfo->pIntChannel = pHAChannel;
   }
   else if (channelId == ismSTORE_HA_CHID_SYNC)
   {
      snprintf(pChParams->ChannelName, 16, "haSBSyncCh");
      pChParams->MessageReceived = ism_store_memHAIntMsgReceived;
      pHAInfo->pSyncChannel = pHAChannel;
      pHAInfo->SyncRC = ISMRC_OK;
   }
   else if (channelId == ismSTORE_HA_CHID_ADMIN)
   {
      snprintf(pChParams->ChannelName, 16, "haSBAdminCh");
      pChParams->MessageReceived = ism_store_memHAIntMsgReceived;
      pHAInfo->pAdminChannel = pHAChannel;
      pHAInfo->fAdminTx = 0;  // Mark the Admin channel as receiver
   }
   else
   {
      pChParams->MessageReceived = ism_store_memHAMsgReceived;
      pHAChannel->AckingPolicy = pHAInfo->AckingPolicy;
      if (flags & 0x1) {
         snprintf(pChParams->ChannelName, 16, "haSBCh.HP.%u", ++pHAInfo->HPChIndex);
      } else {
         snprintf(pChParams->ChannelName, 16, "haSBCh.LP.%u", ++pHAInfo->LPChIndex);
      }
   }
   pthread_mutex_unlock(&pHAInfo->Mutex);

   pChParams->ChannelClosed = ism_store_memHAChannelClosed;
   pChParams->pChContext = (void *)pHAChannel;

   TRACE(5, "An HA channel (Name %s, Id %d, Flags 0x%x, AckingPolicy %u) has been created on the Standby node\n",
         pChParams->ChannelName, channelId, flags, pHAChannel->AckingPolicy);
   TRACE(9, "Exit: %s. ChannelId %d\n", __FUNCTION__, channelId);
   return StoreRC_OK;
}

static int ism_store_memHAChannelClosed(void *hChannel, void *pChContext)
{
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   ismStore_memHAChannel_t *pHAChannel = (ismStore_memHAChannel_t *)pChContext;

   TRACE(9, "Entry: %s. ChannelId %d\n", __FUNCTION__, pHAChannel->ChannelId);
   ism_store_memHAFreeFrags(pHAChannel);

   if (pHAChannel->ChannelId == 0)
   {
      pthread_mutex_lock(&pHAInfo->Mutex);
      pHAInfo->pIntChannel = NULL;
      pthread_mutex_unlock(&pHAInfo->Mutex);
   }
   else if (pHAChannel->ChannelId == ismSTORE_HA_CHID_SYNC)
   {
      pthread_mutex_lock(&pHAInfo->Mutex);
      pHAInfo->pSyncChannel = NULL;
      pthread_mutex_unlock(&pHAInfo->Mutex);
   }
   else  if (pHAChannel->ChannelId == ismSTORE_HA_CHID_ADMIN)
   {
      pthread_mutex_lock(&ismStore_HAAdminMutex);
      pthread_mutex_lock(&pHAInfo->Mutex);
      pHAInfo->pAdminChannel = NULL;
      pthread_mutex_unlock(&pHAInfo->Mutex);
      pthread_mutex_unlock(&ismStore_HAAdminMutex);
   }

   TRACE(5, "The HA channel (ChannelId %d) has been closed on the Standby node\n", pHAChannel->ChannelId);
   ismSTORE_FREE(pHAChannel);

   TRACE(9, "Exit: %s\n", __FUNCTION__);
   return StoreRC_OK;
}

static int ism_store_memHAIntMsgReceived(void *hChannel, char *pData, uint32_t dataLength, void *pChContext)
{
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   ismStore_memHAChannel_t *pHAChannel = (ismStore_memHAChannel_t *)pChContext;
   ismStore_memOperationType_t opType;
   ismStore_memHADiskWriteCtxt_t *pDiskWriteCtxt;
   ismStore_memHAFragment_t *pFrag=NULL, frag;
   ismStore_memHAAck_t ack;
   ismStore_memHAMsgType_t msgType;
   ismStore_memGeneration_t *pGen;
   ismStore_memMgmtHeader_t *pMgmtHeader, *pMgmtHeaderTmp;
   ismStore_memGenHeader_t genHeader, *pGenHeader, *pGenHeaderTmp;
   ismStore_memGranulePool_t *pPool;
   ismStore_memGenToken_t genToken;
   ismStore_memJob_t job;
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_GenId_t genId;
   ismStore_DiskBufferParams_t buffParams;
   ismStore_DiskTaskParams_t diskTask;
   ismHA_AdminMessage_t adminMsg;
   char *ptr, *pPath, *pFilename, *pBuffer=NULL, *pPos, nodeStrP[40], nodeStrS[40], sessionIdStr[32];
   ismStore_HASessionID_t sessionId;
   double syncMemTime, syncTime, compactRatio;
   uint64_t msgLength, offset, tail, diskFileSizeP, diskFileSizeS, diffDiskFileSize;
   uint32_t fragLength, opcount, opcountRes, opLength, argLength=0, bufferLength;
   uint16_t plen, flen;
   uint8_t fGensEqual, genIndex, flags, fFlowCtrlAck, poolId;
   int rc = StoreRC_OK, ec;

   pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;
   ptr = pData;
   memset(&ack, '\0', sizeof(ack));
   memset(&frag, '\0', sizeof(frag));
   ismSTORE_getInt(ptr, fragLength);             // Fragment length
   ismSTORE_getShort(ptr, frag.MsgType);         // Message type
   ismSTORE_getLong(ptr, frag.MsgSqn);           // Message sequence
   ismSTORE_getInt(ptr, frag.FragSqn);           // Fragment sequence
   ismSTORE_getChar(ptr, flags);                 // Fragment flags
   frag.flags = flags;
   fFlowCtrlAck = isFlowCtrlAck(flags);          // Flow control Ack
   ptr += INT_SIZE;                              // Reserved
   ismSTORE_getInt(ptr, opcount);                // Operations count

   TRACE(9, "An HA fragment (ChannelId %d, FragLength %u, MsgType %u, MsgSqn %lu, FragSqn %u, Flags 0x%x, IsLastFrag %d, FlowCtrlAck %u) has been received\n",
         pHAChannel->ChannelId, fragLength, frag.MsgType, frag.MsgSqn, frag.FragSqn, flags, isLastFrag(frag.flags), fFlowCtrlAck);

   if (frag.MsgType == StoreHAMsg_CloseChannel)
   {
      TRACE(7, "The HA channel (ChannelId %u) is being closed by the primary node. MsgSqn %lu, AckSqn %lu\n",
            pHAChannel->ChannelId, frag.MsgSqn, pHAChannel->AckSqn);
      return StoreRC_HA_CloseChannel;
   }

   if (frag.MsgType == StoreHAMsg_SyncError)
   {
      ismSTORE_getShort(ptr, opType);
      ismSTORE_getInt(ptr, opLength);
      if (opType != Operation_Null || opLength != INT_SIZE)
      {
         TRACE(1, "HASync: Failed to receive an HA internal fragment (ChannelId %d, MsgSqn %lu, MsgType %d, opcount %u), because the fragment header is not valid. opType %d, opLength %u\n",
               pHAChannel->ChannelId, frag.MsgSqn, frag.MsgType, opcount, opType, opLength);
         ismStore_global.fSyncFailed = 1;
         return StoreRC_SystemError;
      }

      ismSTORE_getInt(ptr, pHAInfo->SyncRC);
      TRACE(1, "HASync: Failed to synchronize the node into the HA pair. error code %d\n", pHAInfo->SyncRC);
      ismStore_global.fSyncFailed = 1;
      return StoreRC_SystemError;
   }

   pFrag = pHAChannel->pFragTail;
   if (pFrag && (pFrag->MsgSqn != frag.MsgSqn || pFrag->FragSqn + 1 != frag.FragSqn))
   {
      // Make sure that the fragments are received in order
      TRACE(1, "Failed to receive an HA fragment (ChannelId %d, FragLength %u, MsgType %u, MsgSqn %lu, FragSqn %u, IsLastFrag %d, OpCount %u), because the header is not valid. ChannelMsgSqn %lu, ChannelFragSqn %u\n",
            pHAChannel->ChannelId, fragLength, frag.MsgType, frag.MsgSqn, frag.FragSqn, isLastFrag(frag.flags), opcount, pFrag->MsgSqn, pFrag->FragSqn);
      rc = StoreRC_SystemError;
      goto exit;
   }

   // For all the messages on these channels we allocate a single chunk of memory
   // for the whole message, except for the StoreHAMsg_SyncList message.
   // For StoreHAMsg_SyncList message we allocate a memory chunk for each fragment
   if (!pFrag || frag.MsgType == StoreHAMsg_SyncList)
   {
      switch (frag.MsgType)
      {
         case StoreHAMsg_Admin:
            ismSTORE_getShort(ptr, opType);
            ismSTORE_getInt(ptr, opLength);
            if (opType != Operation_Null || opLength != LONG_SIZE)
            {
               TRACE(1, "HAAdmin: Failed to receive an HA internal fragment (ChannelId %d, MsgSqn %lu, MsgType %d, opcount %u), because the fragment header is not valid. opType %d, opLength %u\n",
                     pHAChannel->ChannelId, frag.MsgSqn, frag.MsgType, opcount, opType, opLength);
               rc = StoreRC_SystemError;
               goto exit;
            }

            ismSTORE_getLong(ptr, msgLength);
            break;
         case StoreHAMsg_AdminFile:
            ismSTORE_getShort(ptr, opType);
            ismSTORE_getInt(ptr, opLength);
            if (opType != Operation_Null || opLength < LONG_SIZE)
            {
               TRACE(1, "HAAdmin: Failed to receive an HA internal fragment (ChannelId %d, MsgSqn %lu, MsgType %d, opcount %u), because the fragment header is not valid. opType %d, opLength %u\n",
                     pHAChannel->ChannelId, frag.MsgSqn, frag.MsgType, opcount, opType, opLength);
               rc = StoreRC_SystemError;
               goto exit;
            }

            ismSTORE_getLong(ptr, msgLength);
            argLength = opLength - LONG_SIZE;
            break;
         case StoreHAMsg_SyncList:
            ptr -= INT_SIZE;   // We need to store the opcount in the data
            argLength = dataLength - (uint32_t)(ptr - pData);
            msgLength = 0;
            break;
         case StoreHAMsg_SyncDiskGen:
            ismSTORE_getShort(ptr, opType);
            ismSTORE_getInt(ptr, opLength);
            if (opType != Operation_Null || (opLength != SHORT_SIZE + LONG_SIZE))
            {
               TRACE(1, "HASync: Failed to receive an HA internal fragment (ChannelId %d, MsgSqn %lu, MsgType %d, opcount %u), because the fragment header is not valid. opType %d, opLength %u\n",
                     pHAChannel->ChannelId, frag.MsgSqn, frag.MsgType, opcount, opType, opLength);
               rc = StoreRC_SystemError;
               goto exit;
            }

            ismSTORE_getLong(ptr, msgLength);
            argLength = SHORT_SIZE;

            // Wait until we have enough memory to receive the generation file
            pthread_mutex_lock(&pHAInfo->Mutex);
            while (pHAInfo->State == ismSTORE_HA_STATE_UNSYNC &&   /* BEAM suppression: infinite loop */
                   pHAInfo->SyncRC == ISMRC_OK &&
                   pHAInfo->SyncCurMemSizeBytes + msgLength > pHAInfo->SyncMaxMemSizeBytes &&
                   pHAInfo->SyncCurMemSizeBytes > 0)
            {
               TRACE(9, "HASync: Waits for available memory to receive a generation file (FileSize %lu). SyncMemSizeBytes %lu:%lu\n",
                     msgLength, pHAInfo->SyncCurMemSizeBytes, pHAInfo->SyncMaxMemSizeBytes);
               pthread_cond_wait(&pHAInfo->SyncCond, &pHAInfo->Mutex);
            }

            if (pHAInfo->State != ismSTORE_HA_STATE_UNSYNC || pHAInfo->SyncRC != ISMRC_OK)
            {
               TRACE(5, "HASync: Failed to receive a generation file, because the HA synchronization procedure has been aborted\n");
               pthread_mutex_unlock(&pHAInfo->Mutex);
               goto exit;
            }
            pHAInfo->SyncCurMemSizeBytes += msgLength;
            TRACE(7, "HASync: A memory for a generation file (FileSize %lu) has been allocated. SyncMemSizeBytes %lu:%lu\n",
                  msgLength, pHAInfo->SyncCurMemSizeBytes, pHAInfo->SyncMaxMemSizeBytes);
            pthread_mutex_unlock(&pHAInfo->Mutex);
            break;
         case StoreHAMsg_SyncMemGen:
            ismSTORE_getShort(ptr, opType);
            ismSTORE_getInt(ptr, opLength);
            if (opType != Operation_Null || opLength != (SHORT_SIZE + LONG_SIZE + 1))
            {
               TRACE(1, "HASync: Failed to receive an HA internal fragment (ChannelId %d, MsgSqn %lu, MsgType %d, opcount %u), because the fragment header is not valid. opType %d, opLength %u\n",
                     pHAChannel->ChannelId, frag.MsgSqn, frag.MsgType, opcount, opType, opLength);
               rc = StoreRC_SystemError;
               goto exit;
            }

            ismSTORE_getLong(ptr, msgLength);
            argLength = SHORT_SIZE + 1;
            if (pHAInfo->SyncTime[1] == 0) {
               pHAInfo->SyncTime[1] = ism_common_currentTimeNanos();
            }
            break;
         case StoreHAMsg_SyncComplete:
            argLength = msgLength = 0;
            break;
         default:
            TRACE(1, "Failed to receive an HA internal fragment (ChannelId %d, FragLength %u, MsgType %u, MsgSqn %lu, FragSqn %u, IsLastFrag %d), because the message type is not valid\n",
                  pHAChannel->ChannelId, fragLength, frag.MsgType, frag.MsgSqn, frag.FragSqn, isLastFrag(frag.flags));
            rc = StoreRC_SystemError;
            goto exit;
      }
      // Allocate the first fragment including a memory for the whole message
      if ((pFrag = (ismStore_memHAFragment_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,108),sizeof(ismStore_memHAFragment_t) + msgLength + argLength)) == NULL)
      {
         TRACE(1, "Failed to allocate memory for HA fragment. ChannelId %d, FragLength %u, MsgType %u, MsgSqn %lu, FragSqn %u, IsLastFrag %d, MessageLength %lu\n",
               pHAChannel->ChannelId, fragLength, frag.MsgType, frag.MsgSqn, frag.FragSqn, isLastFrag(frag.flags), msgLength);
         rc = StoreRC_SystemError;
         goto exit;
      }
      memcpy(pFrag, &frag, sizeof(*pFrag));
      pFrag->freeMe = 1 ; 
      pFrag->pData = (char *)pFrag + sizeof(*pFrag);
      pFrag->DataLength = msgLength;
      if (argLength > 0)
      {
         pFrag->pArg = (char *)pFrag->pData + pFrag->DataLength;
         memcpy(pFrag->pArg, ptr, argLength);
         ptr += argLength;
      }
      if (pHAChannel->pFragTail)
      {
         pHAChannel->pFragTail->pNext = pFrag;
         pHAChannel->pFragTail = pFrag;
      }
      else
      {
         pHAChannel->pFrag = pHAChannel->pFragTail = pFrag;
      }
      opcount--;

      TRACE(8, "A memory for an HA large message (MsgLength %lu, ArgLength %u) has been allocated. ChannelId %d, FragLength %u, MsgType %u, MsgSqn %lu, FragSqn %u, IsLastFrag %d\n",
            msgLength, argLength, pHAChannel->ChannelId, fragLength, frag.MsgType, frag.MsgSqn, frag.FragSqn, isLastFrag(frag.flags));
   }

   pFrag->FragSqn = frag.FragSqn;
   pFrag->flags   = frag.flags;

   if (frag.MsgType != StoreHAMsg_SyncList && frag.MsgType != StoreHAMsg_SyncComplete)
   {
      while (opcount-- > 0)
      {
         ismSTORE_getShort(ptr, opType);
         ismSTORE_getInt(ptr, opLength);
         if (opType != Operation_Null || opLength < LONG_SIZE)
         {
            TRACE(1, "Failed to receive an HA internal fragment (ChannelId %d, MsgSqn %lu, MsgType %d, opcount %u), because the fragment header is not valid. opType %d, opLength %u\n",
                  pHAChannel->ChannelId, frag.MsgSqn, frag.MsgType, opcount, opType, opLength);
            rc = StoreRC_SystemError;
            goto exit;
         }

         ismSTORE_getLong(ptr, offset);
         memcpy(pFrag->pData + offset, ptr, opLength - LONG_SIZE);
      }
   }

   if (isLastFrag(frag.flags))
   {
      switch (frag.MsgType)
      {
         case StoreHAMsg_Admin:
            TRACE(9, "HAAdmin: An admin message (MsgSqn %lu, MsgLength %lu) is being forwarded to the Standby node\n",
                  pFrag->MsgSqn, pFrag->DataLength);
            memset(&adminMsg, '\0', sizeof(adminMsg));
            adminMsg.pData = pFrag->pData;
            adminMsg.DataLength = (uint32_t)pFrag->DataLength;
            adminMsg.pResBuffer = pHAInfo->pAdminResBuff;
            adminMsg.ResBufferLength = ismHA_MAX_ADMIN_RESPONSE_LENGTH;
            ism_ha_admin_process_admin_msg(&adminMsg);
            if (adminMsg.ResLength > ismHA_MAX_ADMIN_RESPONSE_LENGTH)
            {
               TRACE(1, "HAAdmin: The admin response data (ResLength %u) has been truncated, because the length exceeds the maximum response length (%u)\n",
                     adminMsg.ResLength, ismHA_MAX_ADMIN_RESPONSE_LENGTH);
               adminMsg.ResLength = ismHA_MAX_ADMIN_RESPONSE_LENGTH;
            }
            ack.DataLength = adminMsg.ResLength;
            ack.pData = pHAInfo->pAdminResBuff;
            ismSTORE_FREE(pFrag);
            break;

         case StoreHAMsg_AdminFile:
            // Extract the path and filename from the pArg
            ptr = pFrag->pArg;
            ismSTORE_getShort(ptr, plen);
            pPath = ptr;
            ptr += plen;
            ismSTORE_getShort(ptr, flen);
            pFilename = ptr;
            ptr += flen;

            if ((pDiskWriteCtxt = (ismStore_memHADiskWriteCtxt_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,109),sizeof(*pDiskWriteCtxt))) == NULL)
            {
               TRACE(1, "HAAdmin: Failed to write an admin file (Path %s, Filename %s, FileSize %lu, MsgSqn %lu) to the Standby disk due to a memory allocation error\n",
                     pPath, pFilename, pFrag->DataLength, pFrag->MsgSqn);
               rc = StoreRC_SystemError;
               pHAChannel->pFrag = pHAChannel->pFragTail = NULL;
               goto exit;
            }

            memset(pDiskWriteCtxt, '\0', sizeof(*pDiskWriteCtxt));
            pDiskWriteCtxt->ViewId = ismStore_memGlobal.HAInfo.View.ViewId;
            pDiskWriteCtxt->pHAChannel = pHAChannel;
            pDiskWriteCtxt->pArg = (void *)pFrag;
            pDiskWriteCtxt->Ack.AckSqn = pFrag->MsgSqn;
            pDiskWriteCtxt->Ack.FragSqn = pFrag->FragSqn;
            pDiskWriteCtxt->Ack.SrcMsgType = pFrag->MsgType;

            memset(&diskTask, '\0', sizeof(diskTask));
            diskTask.Priority = 0;
            diskTask.Callback = ism_store_memHAAdminDiskWriteComplete;
            diskTask.pContext = pDiskWriteCtxt;
            diskTask.Path = pPath;
            diskTask.File = pFilename;
            diskTask.BufferParams->pBuffer = pFrag->pData;
            diskTask.BufferParams->BufferLength = pFrag->DataLength;

            TRACE(9, "HAAdmin: An admin file (Path %s, Filename %s, FileSize %lu, MsgSqn %lu) is being written to the Standby disk\n",
                  pPath, pFilename, pFrag->DataLength, pFrag->MsgSqn);

            if ((rc = ism_storeDisk_writeFile(&diskTask)) != StoreRC_OK)
            {
               TRACE(1, "HAAdmin: Failed to receive an admin file (Path %s, Filename %s, FileSize %lu, MsgSqn %lu) due to a disk failure. error code %d\n",
                     pPath, pFilename, pFrag->DataLength, pFrag->MsgSqn, rc);
               pHAChannel->pFrag = pHAChannel->pFragTail = NULL;
               goto exit;
            }
            break;

         case StoreHAMsg_SyncList:
            memset(pHAInfo->SyncTime, '\0', sizeof(pHAInfo->SyncTime));
            pHAInfo->SyncTime[0] = ism_common_currentTimeNanos();
            msgType = StoreHAMsg_SyncListRes;
            pBuffer = NULL;
            while (pHAChannel->pFrag)
            {
               pFrag = pHAChannel->pFrag;
               ptr = pFrag->pArg;
               ismSTORE_getInt(ptr, opcount);
               while (opcount--)
               {
                  ismSTORE_getShort(ptr, opType);
                  ismSTORE_getInt(ptr, opLength);
                  if (opType != Operation_Null || opLength != (SHORT_SIZE + sizeof(ismStore_memGenToken_t) + LONG_SIZE))
                  {
                     TRACE(1, "HASyncList: Failed to receive an HA internal fragment (ChannelId %d, MsgSqn %lu, MsgType %d, opcount %u), because the fragment header is not valid. opType %d, opLength %u\n",
                           pHAChannel->ChannelId, frag.MsgSqn, frag.MsgType, opcount, opType, opLength);
                     rc = StoreRC_SystemError;
                     pHAChannel->pFrag = pHAChannel->pFragTail = NULL;
                     goto exit;
                  }

                  ismSTORE_getShort(ptr, genId);
                  memcpy(&genToken, ptr, sizeof(ismStore_memGenToken_t));
                  ptr += sizeof(ismStore_memGenToken_t);
                  ismSTORE_getLong(ptr, diskFileSizeP);

                  if (genId == ismSTORE_MGMT_GEN_ID)
                  {
                     memcpy(&genHeader.GenToken, &pMgmtHeader->GenToken, sizeof(ismStore_memGenToken_t));
                     diskFileSizeS = 0;
                  }
                  else
                  {
                     if ((ec = ism_storeDisk_getGenerationInfo(genId, sizeof(genHeader), (char *)&genHeader, &diskFileSizeS)) != StoreRC_OK)
                     {
                        TRACE(5, "HASyncList: Failed to retrieve the generation file information (GenId %u) from the Standby disk. error code %d\n", genId, ec);
                        memset(&genHeader, '\0', sizeof(genHeader));
                        diskFileSizeS = 0;
                     }

                     TRACE(7, "HASyncList: Generation file (GenId %u) information on the Standby: GenId %u, Version %lu, MemSizeBytes %lu, PoolsCount %u, State %u, DescriptorStructSize %u, SplitItemStructSize %u, RsrvPoolMemSizeBytes %lu\n",
                           genId, genHeader.GenId, genHeader.Version, genHeader.MemSizeBytes, genHeader.PoolsCount,
                           genHeader.State, genHeader.DescriptorStructSize, genHeader.SplitItemStructSize, genHeader.RsrvPoolMemSizeBytes);
                  }

                  ism_store_memB2H(nodeStrP, (uint8_t *)(genToken.NodeId), sizeof(ismStore_HANodeID_t));
                  ism_store_memB2H(nodeStrS, (uint8_t *)(genHeader.GenToken.NodeId), sizeof(ismStore_HANodeID_t));
                  diffDiskFileSize = (diskFileSizeP > diskFileSizeS ? diskFileSizeP - diskFileSizeS : diskFileSizeS - diskFileSizeP);
                  fGensEqual = ((genToken.Timestamp > 0) && (memcmp(&genToken, &genHeader.GenToken, sizeof(ismStore_memGenToken_t)) == 0) && (diffDiskFileSize < ismSTORE_HA_SYNC_DIFF));

                  TRACE(7, "HASyncList: Comparing generation files (GenId %u) on Primary (GenToken %s:%lu, DiskFileSize %lu) and Standby (GenToken %s:%lu, DiskFileSize %lu). fGensEqual %u, diffFileSize %lu\n",
                        genId, nodeStrP, genToken.Timestamp, diskFileSizeP, nodeStrS, genHeader.GenToken.Timestamp, diskFileSizeS, fGensEqual, diffDiskFileSize);

                  if (diskFileSizeS > 0 && fGensEqual)
                  {
                     // Read-ahead: If the generation file already exists on the disk, we want to read it into the Standby memory for fast recovery
                     if ((ec = ism_store_memRecoveryAddGeneration(genId, NULL, 0, 0)) != ISMRC_OK)
                     {
                        TRACE(1, "HASyncList: Failed to read ahead a generation file (GenId %u, DiskFileSize %lu) into the Standby memory. error code %d\n",
                              genId, diskFileSizeS, ec);
                      }
                  }

                  if (!fGensEqual || !pBuffer)
                  {
                     if ((ec = ism_store_memHAEnsureBufferAllocation(pHAChannel,
                                                                     &pBuffer,
                                                                     &bufferLength,
                                                                     &pPos,
                                                                     64,
                                                                     msgType,
                                                                     &opcountRes)) != StoreRC_OK)
                     {
                        TRACE(1, "HASyncList: Failed to send a message (MsgType %u, MsgSqn %lu, LastFrag %u) to the Primary node. error code %d\n",
                              msgType, pHAChannel->MsgSqn, pHAChannel->FragSqn, ec);
                        rc = StoreRC_SystemError;
                        pHAChannel->pFrag = pHAChannel->pFragTail = NULL;
                        goto exit;
                     }

                     if (!fGensEqual)
                     {
                        ismSTORE_putShort(pPos, Operation_Null);
                        ismSTORE_putInt(pPos, SHORT_SIZE);
                        ismSTORE_putShort(pPos, genId);
                        TRACE(5, "HASyncList: Adding generation (GenId %u, DiskFileSize %lu) to the requested list. Index %u\n", genId, diskFileSizeS, opcountRes);
                        opcountRes++;
                     }
                  }
               }
               pHAChannel->pFrag = pFrag->pNext;
               ismSTORE_FREE(pFrag);
            }

            // Mark the store role as Unsync
            TRACE(5, "Store role has been changed from %d to %d\n", pMgmtHeader->Role, ismSTORE_ROLE_UNSYNC);
            pMgmtHeader->Role = ismSTORE_ROLE_UNSYNC;
            pMgmtHeader->WasPrimary = 0;
            pMgmtHeader->HaveData = 0;
            pMgmtHeader->PrimaryTime = 0;
            ADR_WRITE_BACK(&pMgmtHeader->Role, 16);

            // Send the last fragment
            // requiredLength = 0 means that this is the last fragment.
            if ((ec = ism_store_memHAEnsureBufferAllocation(pHAChannel, &pBuffer, &bufferLength, &pPos, 0, msgType, &opcountRes)) != StoreRC_OK)
            {
               TRACE(1, "HASyncList: Failed to send a message (MsgType %u, MsgSqn %lu, LastFrag %u) to the Primary node. error code %d\n",
                     msgType, pHAChannel->MsgSqn, pHAChannel->FragSqn, ec);
               rc = StoreRC_SystemError;
               pHAChannel->pFrag = pHAChannel->pFragTail = NULL;
               goto exit;
            }

            // We reset the in-memory generations at this point, because we don't want to do that when the store is locked
            memset(ismStore_memGlobal.MgmtGen.pBaseAddress + sizeof(ismStore_memMgmtHeader_t), '\0', ismStore_memGlobal.TotalMemSizeBytes - sizeof(ismStore_memMgmtHeader_t));
            break;

         case StoreHAMsg_SyncDiskGen:
            // Extract the GenId from the pArg
            ptr = pFrag->pArg;
            ismSTORE_getShort(ptr, genId);

            if ((ec = ism_store_memRecoveryAddGeneration(genId, pFrag->pData, pFrag->DataLength, 0)) != ISMRC_OK)
            {
               TRACE(1, "HASync: Failed to receive a generation file (GenId %u, FileSize %lu, MsgSqn %lu) due to a recovery failure. error code %d\n",
                     genId, pFrag->DataLength, pFrag->MsgSqn, ec);
               rc = StoreRC_SystemError;
               pHAChannel->pFrag = pHAChannel->pFragTail = NULL;
               goto exit;
            }

            memset(&diskTask, '\0', sizeof(diskTask));
            diskTask.GenId = genId;
            diskTask.Priority = 0;
            diskTask.Callback = ism_store_memHASyncDiskWriteComplete;
            diskTask.pContext = pFrag;
            diskTask.BufferParams->pBuffer = pFrag->pData;
            diskTask.BufferParams->BufferLength = pFrag->DataLength;

            TRACE(5, "HASync: A generation file (GenId %u, FileSize %lu, MsgSqn %lu) is being written to the Standby disk\n",
                  genId, pFrag->DataLength, pFrag->MsgSqn);

            if ((rc = ism_storeDisk_writeGeneration(&diskTask)) != StoreRC_OK)
            {
               TRACE(1, "HASync: Failed to receive a generation file (GenId %u, FileSize %lu, MsgSqn %lu) due to a disk failure. error code %d\n",
                     genId, pFrag->DataLength, pFrag->MsgSqn, rc);
               pHAChannel->pFrag = pHAChannel->pFragTail = NULL;
               goto exit;
            }
            break;

         case StoreHAMsg_SyncMemGen:
            // Extract the GenId from the pArg
            ptr = pFrag->pArg;
            ismSTORE_getShort(ptr, genId);
            ismSTORE_getChar(ptr, genIndex);
            if (genId != ismSTORE_MGMT_GEN_ID && genIndex >= ismStore_memGlobal.InMemGensCount)
            {
               TRACE(1, "HASync: Failed to receive an HA internal fragment (ChannelId %d, MsgSqn %lu, MsgType %d, opcount %u), because the fragment header is not valid. GenId %u, GenIndex %u\n",
                     pHAChannel->ChannelId, frag.MsgSqn, frag.MsgType, opcount, genId, genIndex);
               rc = StoreRC_SystemError;
               ismSTORE_FREE(pFrag);
               pHAChannel->pFrag = pHAChannel->pFragTail = NULL;
               goto exit;
            }

            if (genId == ismSTORE_MGMT_GEN_ID)
            {
               // We do not want to change the store role on the Standby node
               pMgmtHeaderTmp = (ismStore_memMgmtHeader_t *)pFrag->pData;
               pMgmtHeaderTmp->Role = ismSTORE_ROLE_UNSYNC;
               pMgmtHeaderTmp->WasPrimary = 0;
               pMgmtHeaderTmp->PrimaryTime = 0;
               memset(sessionId, 0, sizeof(ismStore_HASessionID_t));
               if (memcmp(pMgmtHeaderTmp->SessionId, sessionId, sizeof(ismStore_HASessionID_t)) != 0)
               {
                  ism_store_memB2H(sessionIdStr, (uint8_t *)(pMgmtHeaderTmp->SessionId), sizeof(ismStore_HASessionID_t));
                  TRACE(5, "HASync: The SessionId of the Primary node is not empty. SessionId %s, SessionCount %u\n", sessionIdStr, pMgmtHeaderTmp->SessionCount);
                  pMgmtHeaderTmp->SessionCount -= 1;
               }
               if ( pMgmtHeaderTmp->TotalMemSizeBytes != ismStore_memGlobal.TotalMemSizeBytes )
               {
                  ismStore_global.PrimaryMemSizeBytes = pMgmtHeaderTmp->TotalMemSizeBytes ; 
                  TRACE(1, "HASync: Failed to accept the management generation received from the Primary because TotalMemSizeBytes mismatch (PR %lu, SB %lu).\n",
                        pMgmtHeaderTmp->TotalMemSizeBytes,ismStore_memGlobal.TotalMemSizeBytes);
                  rc = StoreRC_SystemError;
                  ismSTORE_FREE(pFrag);
                  pHAChannel->pFrag = pHAChannel->pFragTail = NULL;
                  goto exit;
               }
            }

            pGen = (genId == ismSTORE_MGMT_GEN_ID ? &ismStore_memGlobal.MgmtGen : &ismStore_memGlobal.InMemGens[genIndex]);
            pGenHeader = (ismStore_memGenHeader_t *)pGen->pBaseAddress;

            // See whether the generation data is compacted. If yes, we need to expand the data
            pGenHeaderTmp = (ismStore_memGenHeader_t *)pFrag->pData;
            if (pGenHeaderTmp->CompactSizeBytes > 0)
            {
               memset(&buffParams, '\0', sizeof(buffParams));
               buffParams.pBuffer = pGen->pBaseAddress;
               buffParams.BufferLength = pGenHeaderTmp->MemSizeBytes;
               if ((rc = ism_storeDisk_expandGenerationData(pFrag->pData, &buffParams)) != StoreRC_OK)
               {
                  TRACE(1, "HASync: Failed to expand the memory generation data (GenId %u, GenIndex %u, CompactedSizeBytes %lu). ChannelId %d, MsgSqn %lu, MsgType %d, DataLength %lu, opcount %u, error code %d\n",
                        genId, genIndex, pGenHeaderTmp->CompactSizeBytes, pHAChannel->ChannelId, frag.MsgSqn, frag.MsgType, pFrag->DataLength, opcount, rc);
                  ismSTORE_FREE(pFrag);
                  pHAChannel->pFrag = pHAChannel->pFragTail = NULL;
                  goto exit;
               }

               // Initialize the PoolId for the granules in pool Id 1 and up, because the expandGenerationData
               // copies only the non-empty granules
               for (poolId=1; poolId < pGenHeader->PoolsCount; poolId++)
               {
                  pPool = &pGenHeader->GranulePool[poolId];
                  for (offset = pPool->Offset, tail = pPool->Offset + pPool->MaxMemSizeBytes; offset < tail; offset += pPool->GranuleSizeBytes)
                  {
                     pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + offset);
                     pDescriptor->PoolId = poolId;
                  }
               }

               compactRatio = 1.0 - (double)pFrag->DataLength / pGenHeader->MemSizeBytes;
               TRACE(5, "HASync: The memory generation (GenId %u, GenIndex %u, CompactedSizeBytes %lu, MemSizeBytes %lu) has been expanded. DataLength %lu, CompactRatio %f\n",
                   genId, genIndex, pGenHeaderTmp->CompactSizeBytes, pGenHeader->MemSizeBytes, pFrag->DataLength, compactRatio);
            }
            else
            {
               memcpy(pGen->pBaseAddress, pFrag->pData, pFrag->DataLength);
            }

            if (pGenHeader->MemSizeBytes > ismSTORE_HA_SYNC_FLUSH) {
               ADR_WRITE_BACK(pGen->pBaseAddress + (pGenHeader->MemSizeBytes - ismSTORE_HA_SYNC_FLUSH), ismSTORE_HA_SYNC_FLUSH);
            } else {
               ADR_WRITE_BACK(pGen->pBaseAddress, pGenHeader->MemSizeBytes);
            }

            if (genId != ismSTORE_MGMT_GEN_ID)
            {
               pGen->MaxRefsPerGranule = (pGenHeader->GranulePool[0].GranuleSizeBytes - sizeof(ismStore_memDescriptor_t) -
                                          offsetof(ismStore_memReferenceChunk_t, References)) / sizeof(ismStore_memReference_t);
            }

            TRACE(5, "HASync: The memory generation (GenId %u, GenIndex %u, DataLength %lu, MsgSqn %lu) has been received by the Standby node. " \
                 "GenId %u, Version %lu, MemSizeBytes %lu, CompactedSizeBytes %lu, PoolsCount %u, State %u, DescriptorStructSize %u, SplitItemStructSize %u, " \
                 "RsrvPoolMemSizeBytes %lu, MaxRefsPerGranule %u\n",
                 genId, genIndex, pFrag->DataLength, pFrag->MsgSqn, pGenHeader->GenId,
                 pGenHeader->Version, pGenHeader->MemSizeBytes, pGenHeader->CompactSizeBytes,
                 pGenHeader->PoolsCount, pGenHeader->State, pGenHeader->DescriptorStructSize,
                 pGenHeader->SplitItemStructSize,  pGenHeader->RsrvPoolMemSizeBytes, pGen->MaxRefsPerGranule);

            if (pGenHeader->State == ismSTORE_GEN_STATE_FREE)
            {
               if (pFrag->DataLength != sizeof(ismStore_memGenHeader_t))
               {
                  TRACE(1, "HASync: The memory generation (GenId %u, Index %u) is corrupted. DataLength %lu\n", genId, genIndex, pFrag->DataLength);
               }
               for (poolId=0; poolId < pGenHeader->PoolsCount; poolId++) {
                  ism_store_memPreparePool(pGenHeader->GenId, pGen, &pGenHeader->GranulePool[poolId], poolId, 1);
               }
               TRACE(5, "HASync: A free memory generation (GenId %u, Index %u) has been initialized on the Standby node\n", pGenHeader->GenId, genIndex);
            }

            ismSTORE_FREE(pFrag);
            break;

         case StoreHAMsg_SyncComplete:
            ismSTORE_FREE(pFrag);
            pthread_mutex_lock(&pHAInfo->Mutex);
            if (pHAInfo->State != ismSTORE_HA_STATE_UNSYNC || pHAInfo->SyncRC != ISMRC_OK)
            {
               TRACE(1, "HASync: The new node synchronization procedure has been aborted. State %d, SyncRC %d\n", pHAInfo->State, pHAInfo->SyncRC);
               rc = (pHAInfo->SyncRC == ISMRC_OK ? StoreRC_SystemError : pHAInfo->SyncRC);
               pthread_mutex_unlock(&pHAInfo->Mutex);
               pHAChannel->pFrag = pHAChannel->pFragTail = NULL;
               goto exit;
            }

            if (ism_store_memValidate() != ISMRC_OK)
            {
               TRACE(1, "HASync: The management generation on the Standby node is not valid\n");
               rc = StoreRC_SystemError;
               pthread_mutex_unlock(&pHAInfo->Mutex);
               pHAChannel->pFrag = pHAChannel->pFragTail = NULL;
               goto exit;
            }

            pHAInfo->State = ismSTORE_HA_STATE_STANDBY;
            pHAInfo->SyncNodesCount = 2;
            ism_store_memHASetHasStandby() ; 
            pHAInfo->View.NewRole = ISM_HA_ROLE_STANDBY;
            memset(&job, '\0', sizeof(job));
            job.JobType = StoreJob_HAViewChanged;
            job.HAView.ViewId = pHAInfo->View.ViewId;
            job.HAView.NewRole = pHAInfo->View.NewRole;
            job.HAView.OldRole = pHAInfo->View.OldRole;
            job.HAView.ActiveNodesCount = pHAInfo->View.ActiveNodesCount;
            pthread_mutex_unlock(&pHAInfo->Mutex);

            pHAInfo->SyncTime[2] = ism_common_currentTimeNanos();
            syncMemTime = (double)(pHAInfo->SyncTime[2] - pHAInfo->SyncTime[1]) / 1e9;
            syncTime = (double)(pHAInfo->SyncTime[2] - pHAInfo->SyncTime[0]) / 1e9;

            // Mark the store role as Standby
            TRACE(5, "Store role has been changed from %d to %d\n", pMgmtHeader->Role, ismSTORE_ROLE_STANDBY);
            pMgmtHeader->Role = ismSTORE_ROLE_STANDBY;
            pMgmtHeader->WasPrimary = 0;
            pMgmtHeader->HaveData = 1;
            pMgmtHeader->PrimaryTime = 0;
            ADR_WRITE_BACK(&pMgmtHeader->Role, 16); // from Role to PrimaryTime inclusive

            // Delete any unnecessary generation file from the Standby disk
            memset(&diskTask, '\0', sizeof(diskTask));
            diskTask.fCancelOnTerm = 1;
            diskTask.Priority = 0;
            diskTask.Callback = ism_store_memHADiskDeleteDeadComplete;
            if ((ec = ism_storeDisk_deleteDeadFiles(&diskTask)) != StoreRC_OK)
            {
               TRACE(1, "HASync: Failed to delete dead generation files from the Standby disk. error code %d\n", ec);
            }

            TRACE(5, "HASync: The new node synchronization procedure has been completed successfully. SyncTime %.6f (memTime %.6f), NewRole %u, OldRole %u, ActiveNodesCount %u, SyncNodesCount %u\n",
                  syncTime, syncMemTime, pHAInfo->View.NewRole, pHAInfo->View.OldRole, pHAInfo->View.ActiveNodesCount, pHAInfo->SyncNodesCount);

            ism_store_memAddJob(&job);
            break;

         default:
            ismSTORE_FREE(pFrag);
            break;
      }
      pHAChannel->pFrag = pHAChannel->pFragTail = NULL;
   }

exit:
   // Send back an ACK
   if (rc != StoreRC_OK || (isLastFrag(frag.flags) && frag.MsgType != StoreHAMsg_AdminFile && frag.MsgType != StoreHAMsg_SyncDiskGen && frag.MsgType != StoreHAMsg_SyncList) || fFlowCtrlAck)
   {
      ack.AckSqn = frag.MsgSqn;
      ack.FragSqn = frag.FragSqn;
      ack.SrcMsgType = frag.MsgType;
      ack.ReturnCode = rc;
      if ((ec = ism_store_memHASendAck(pHAChannel, &ack)) != StoreRC_OK)
      {
         TRACE(1, "Failed to send ACK for HA message (ChannelId %d, MsgType %u, MsgSqn %lu, FragSqn %u, LastFrag %u, DataLength %lu, OpCount %u, rc %d). error code %d\n",
               pHAChannel->ChannelId, frag.MsgType, frag.MsgSqn, frag.FragSqn, isLastFrag(frag.flags), frag.DataLength, opcount, rc, ec);
         if (rc == StoreRC_OK)
         {
            rc = ec;
         }
      }
   }

   return rc;
}

static int ism_store_memHAMsgReceived(void *hChannel, char *pData, uint32_t dataLength, void *pChContext)
{
   ismStore_memHAChannel_t *pHAChannel = (ismStore_memHAChannel_t *)pChContext;
   ismStore_memHAFragment_t *pFrag, frag;
   ismStore_memHAAck_t ack;
   uint32_t fragLength;
   uint8_t flags, fFlowCtrlAck;
   char *ptr;
   int rc = StoreRC_OK;

   ptr = pData;
   memset(&frag, '\0', sizeof(frag));
   ismSTORE_getInt(ptr, fragLength);             // Fragment length
   ismSTORE_getShort(ptr, frag.MsgType);         // Message type
   ismSTORE_getLong(ptr, frag.MsgSqn);           // Message sequence
   ismSTORE_getInt(ptr, frag.FragSqn);           // Fragment sequence
   ismSTORE_getChar(ptr, flags);                 // Fragment flags
   frag.flags = flags;
   fFlowCtrlAck = isFlowCtrlAck(flags);          // Flow control Ack
   ptr += INT_SIZE;                              // Reserved
   frag.pData = ptr;
   frag.DataLength = dataLength - (uint32_t)(ptr - pData);

   // Make sure that the fragments are received in order
   if (pHAChannel->pFragTail && (pHAChannel->pFragTail->MsgSqn != frag.MsgSqn || pHAChannel->pFragTail->FragSqn + 1 != frag.FragSqn))
   {
      TRACE(1, "Failed to receive an HA message fragment (ChannelId %d, FragLength %u, MsgType %u, MsgSqn %lu, FragSqn %u, IsLastFrag %d), because the header is not valid. ChannelMsgSqn %lu, ChannelFragSqn %u\n",
            pHAChannel->ChannelId, fragLength, frag.MsgType, frag.MsgSqn, frag.FragSqn, isLastFrag(frag.flags), pHAChannel->pFragTail->MsgSqn, pHAChannel->pFragTail->FragSqn);
      return StoreRC_SystemError;
   }

   TRACE(9, "An HA fragment (ChannelId %d, FragLength %u, MsgType %u, MsgSqn %lu, FragSqn %u, Flags 0x%x, IsLastFrag %d, FlowCtrlAck %u, DataLength %lu) has been received\n",
         pHAChannel->ChannelId, fragLength, frag.MsgType, frag.MsgSqn, frag.FragSqn, flags, isLastFrag(frag.flags), fFlowCtrlAck, frag.DataLength);

   if (frag.MsgType == StoreHAMsg_CloseChannel)
   {
      TRACE(7, "The HA channel (ChannelId %u) is being closed by the primary node. MsgSqn %lu, AckSqn %lu\n",
            pHAChannel->ChannelId, frag.MsgSqn, pHAChannel->AckSqn);
      return StoreRC_HA_CloseChannel;
   }

   if (isLastFrag(frag.flags))
   {
      // See whether we have to send ACK before the processing
      if (pHAChannel->AckingPolicy == 0 && !isNoAck(flags))
      {
         memset(&ack, '\0', sizeof(ack));
         ack.AckSqn = frag.MsgSqn;
         ack.FragSqn = frag.FragSqn;
         ack.SrcMsgType = frag.MsgType;
         ack.ReturnCode = StoreRC_OK;
         if ((rc = ism_store_memHASendAck(pHAChannel, &ack)) != StoreRC_OK)
         {
            TRACE(1, "Failed to send ACK for HA message (ChannelId %d, MsgType %u, MsgSqn %lu, FragSqn %u, LastFrag %u, DataLength %lu). error code %d\n",
                  pHAChannel->ChannelId, frag.MsgType, frag.MsgSqn, frag.FragSqn, isLastFrag(frag.flags), frag.DataLength, rc);
            return rc;
         }
      }

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

      if (frag.MsgType == StoreHAMsg_CreateGen ||
          frag.MsgType == StoreHAMsg_ActivateGen ||
          frag.MsgType == StoreHAMsg_WriteGen ||
          frag.MsgType == StoreHAMsg_DeleteGen ||
          frag.MsgType == StoreHAMsg_CompactGen ||
          frag.MsgType == StoreHAMsg_AssignRsrvPool ||
          frag.MsgType == StoreHAMsg_Shutdown)
      {
         rc = ism_store_memHAParseGenMsg(pHAChannel);
      }
      else
      {
         rc = ism_store_memHAParseMsg(pHAChannel);
      }

      // Release the memory of the fragments (except the last fragment which is located on the stack).
      ism_store_memHAFreeFrags(pHAChannel);
   }
   else
   {
      if ((pFrag = (ismStore_memHAFragment_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,110),sizeof(ismStore_memHAFragment_t) + frag.DataLength)) == NULL)
      {
         TRACE(1, "Failed to allocate memory for HA fragment. ChannelId %d, FragLength %u, MsgType %u, MsgSqn %lu, FragSqn %u, IsLastFrag %d, DataLength %lu\n",
               pHAChannel->ChannelId, fragLength, frag.MsgType, frag.MsgSqn, frag.FragSqn, isLastFrag(frag.flags), frag.DataLength);
         return StoreRC_SystemError;
      }
      memcpy(pFrag, &frag, sizeof(ismStore_memHAFragment_t));
      pFrag->freeMe = 1 ; 
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

      // See whether we have to send back a flow control ACK
      if (fFlowCtrlAck)
      {
         memset(&ack, '\0', sizeof(ack));
         ack.AckSqn = frag.MsgSqn;
         ack.FragSqn = frag.FragSqn;
         ack.SrcMsgType = frag.MsgType;
         ack.ReturnCode = StoreRC_OK;
         if ((rc = ism_store_memHASendAck(pHAChannel, &ack)) != StoreRC_OK)
         {
            TRACE(1, "Failed to send a flow control ACK for HA message (ChannelId %d, MsgType %u, MsgSqn %lu, FragSqn %u, LastFrag %u, DataLength %lu). error code %d\n",
                  pHAChannel->ChannelId, frag.MsgType, frag.MsgSqn, frag.FragSqn, isLastFrag(frag.flags), frag.DataLength, rc);
            return rc;
         }
      }
   }

   return rc;
}

static int ism_store_memHAParseGenMsg(ismStore_memHAChannel_t *pHAChannel)
{
   ismStore_memHAFragment_t *pFrag;
   ismStore_memHAMsgType_t msgType;
   ismStore_memOperationType_t opType;
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memGenHeader_t *pGenHeader;
   ismStore_memGeneration_t *pGen;
   ismStore_memGranulePool_t *pPool;
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_memGenMap_t genMap;
   ismStore_memHAAck_t ack;
   ismStore_DiskTaskParams_t diskTask;
   ismStore_memHADiskWriteCtxt_t *pDiskWriteCtxt = NULL;
   ismStore_Handle_t handle;
   ismStore_GenId_t genId=0;
   uint8_t genIndex, genIndexPrev, poolId, bitMapIndex;
   uint32_t opcount, opLength, dataLength, offset, granulesCount=0;
   uint64_t bitmask, *pBitMap, **pBitMaps = NULL, *pBitMapsArray[ismSTORE_GRANULE_POOLS_COUNT];
   double compactRatio;
   char *pData, *pBase, nodeStr[40];
   int rc = StoreRC_OK, ec, i, n;

   pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;
   memset(&ack, '\0', sizeof(ack));
   memset(&genMap, '\0', sizeof(genMap));
   pFrag = pHAChannel->pFrag;
   msgType = pFrag->MsgType;
   pData = pFrag->pData;

   ismSTORE_getInt(pData, opcount);           // Operations count
   TRACE(9, "Parsing an HA generation message. ChannelId %d, MsgType %u, MsgSqn %lu, " \
         "FragSqn %u, IsLastFrag %u, LastFrag %u, DataLength %lu, OpCount %u\n",
         pHAChannel->ChannelId, msgType, pFrag->MsgSqn, pFrag->FragSqn,
         isLastFrag(pFrag->flags), pHAChannel->pFragTail->FragSqn, pFrag->DataLength, opcount);

   ismSTORE_getShort(pData, opType);          // Operation type
   ismSTORE_getInt(pData, opLength);          // Operation data length
   if (opType != Operation_Null || opLength < SHORT_SIZE + 1)
   {
      TRACE(1, "Failed to parse an HA generation message. ChannelId %d, MsgType %u, MsgSqn %lu, FragSqn %u, " \
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
      TRACE(1, "Failed to parse an HA fragment. ChannelId %d, MsgType %u, MsgSqn %lu, FragSqn %u, " \
            "IsLastFrag %u, DataLength %lu, OpCount %u, OpType %d, OpDataLength %u, GenId %u, GenIndex %u\n",
            pHAChannel->ChannelId, pFrag->MsgType, pFrag->MsgSqn, pFrag->FragSqn, isLastFrag(pFrag->flags),
            pFrag->DataLength, opcount, opType, opLength, genId, genIndex);
      rc = StoreRC_SystemError;
      goto exit;
   }
   opcount--;

   TRACE(5, "An HA generation message (GenId %u, Index %u) has been received. ChannelId %d, MsgType %u, MsgSqn %lu, FragSqn %u, IsLastFrag %u\n",
         genId, genIndex, pHAChannel->ChannelId, pFrag->MsgType, pFrag->MsgSqn, pFrag->FragSqn, isLastFrag(pFrag->flags));

   if (pFrag->MsgType == StoreHAMsg_CreateGen)
   {
      // We set the GenId here to allow the memMapHandleToAddress to work properly for the
      // next operation of this fragment, which contains the new generation header
      pGenHeader = (ismStore_memGenHeader_t *)ismStore_memGlobal.InMemGens[genIndex].pBaseAddress;
      pGenHeader->GenId = genId;
      ADR_WRITE_BACK(&pGenHeader->GenId, sizeof(pGenHeader->GenId));
   }

   while (1)
   {
      for (i=0; i < opcount; i++)
      {
         pBase = pData;
         ismSTORE_getShort(pData, opType);       // Operation type
         ismSTORE_getInt(pData, opLength);       // Operation data length

         switch (opType)
         {
            case Operation_CreateRecord:
               ismSTORE_getLong(pData, handle);
               pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(handle);
               memcpy(pDescriptor, pData, opLength - LONG_SIZE);
               ADR_WRITE_BACK(pDescriptor, opLength - LONG_SIZE);
               pData += (opLength - LONG_SIZE);
               break;

            case Operation_AllocateGranulesMap:
               ismSTORE_getLong(pData, genMap.PredictedSizeBytes);
               ismSTORE_getChar(pData, genMap.GranulesMapCount);
               if (genMap.GranulesMapCount == 0 || genMap.GranulesMapCount > ismSTORE_GRANULE_POOLS_COUNT)
               {
                  TRACE(1, "Failed to parse an HA generation message, because the GranulesMapCount (%u) is not valid. " \
                        "ChannelId %d, MsgType %u, MsgSqn %lu, FragSqn %u, IsLastFrag %u, DataLength %lu, OpCount %u, " \
                        "OpType %d, OpDataLength %u\n",
                        genMap.GranulesMapCount, pHAChannel->ChannelId, pFrag->MsgType, pFrag->MsgSqn, pFrag->FragSqn,
                        isLastFrag(pFrag->flags), pFrag->DataLength, opcount, opType, opLength);
                   rc = StoreRC_SystemError;
                   goto exit;
               }
               TRACE(7, "Generation map (GenId %u, Index %u, PredictedSizeBytes %lu, GranulesMapCount %u) has been allocated on the Standby node\n",
                     genId, genIndex, genMap.PredictedSizeBytes, genMap.GranulesMapCount);
               break;

            case Operation_CreateGranulesMap:
               ismSTORE_getChar(pData, poolId);
               ismSTORE_getChar(pData, bitMapIndex);
               ismSTORE_getInt(pData, dataLength);
               if (poolId >= genMap.GranulesMapCount || bitMapIndex >= ismSTORE_BITMAPS_COUNT)
               {
                  TRACE(1, "Failed to parse an HA generation fragment, due to an internal error. ChannelId %d, MsgType %u, MsgSqn %lu, FragSqn %u, IsLastFrag %u, DataLength %lu, OpCount %u, OpDataLength %u, PoolId %u, GranulesMapCount %u, bitMapIndex %u\n",
                        pHAChannel->ChannelId, pFrag->MsgType, pFrag->MsgSqn, pFrag->FragSqn, isLastFrag(pFrag->flags), pFrag->DataLength, opcount, opLength, poolId, genMap.GranulesMapCount, bitMapIndex);
                  rc = StoreRC_SystemError;
                  goto exit;
               }

               if ((genMap.GranulesMap[poolId].pBitMap[bitMapIndex] = (uint64_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,111),dataLength)) == NULL)
               {
                  TRACE(1, "Failed to parse an HA generation fragment, due to a memory allocation error (%u). ChannelId %d, MsgType %u, MsgSqn %lu, FragSqn %u, IsLastFrag %u, DataLength %lu, OpCount %u, OpDataLength %u, bitMapIndex %u\n",
                        dataLength, pHAChannel->ChannelId, pFrag->MsgType, pFrag->MsgSqn, pFrag->FragSqn, isLastFrag(pFrag->flags), pFrag->DataLength, opcount, opLength, bitMapIndex);
                  rc = StoreRC_SystemError;
                  goto exit;
               }
               memset(genMap.GranulesMap[poolId].pBitMap[bitMapIndex], '\0', dataLength);
               genMap.GranulesMap[poolId].BitMapSize = dataLength / sizeof(uint64_t);
               TRACE(7, "Generation bitmap (GenId %u, Index %u, PoolId %u, BitMapIndex %u) has been allocated on the Standby node. BitMapSize %u\n",
                     genId, genIndex, poolId, bitMapIndex, genMap.GranulesMap[poolId].BitMapSize);
               break;

            case Operation_SetGranulesMap:
               ismSTORE_getChar(pData, poolId);
               ismSTORE_getChar(pData, bitMapIndex);
               if (poolId < genMap.GranulesMapCount && bitMapIndex < ismSTORE_BITMAPS_COUNT && genMap.GranulesMap[poolId].pBitMap[bitMapIndex])
               {
                  ismSTORE_getInt(pData, offset);
                  memcpy((char *)genMap.GranulesMap[poolId].pBitMap[bitMapIndex] + offset, pData, opLength - (INT_SIZE + 2));
               }
               else
               {
                   TRACE(1, "Failed to parse an HA generation fragment, due to an internal error. ChannelId %d, MsgType %u, MsgSqn %lu, FragSqn %u, IsLastFrag %u, DataLength %lu, OpCount %u, OpDataLength %u, PoolId %u, GranulesMapCount %u, bitMapIndex %u\n",
                         pHAChannel->ChannelId, pFrag->MsgType, pFrag->MsgSqn, pFrag->FragSqn, isLastFrag(pFrag->flags), pFrag->DataLength, opcount, opLength, poolId, genMap.GranulesMapCount, bitMapIndex);
                   rc = StoreRC_SystemError;
                   goto exit;
               }
               break;

            default:
               TRACE(1, "Failed to parse a store operation in HA generation fragment, because the operation type %d is not valid. ChannelId %d, MsgType %u, MsgSqn %lu, FragSqn %u, IsLastFrag %u, DataLength %lu, OpCount %u, OpDataLength %u\n",
                     opType, pHAChannel->ChannelId, pFrag->MsgType, pFrag->MsgSqn, pFrag->FragSqn, isLastFrag(pFrag->flags), pFrag->DataLength, opcount, opLength);
               rc = StoreRC_SystemError;
               goto exit;
         }
         pData = pBase + (SHORT_SIZE + INT_SIZE + opLength);
      }

      if ((pFrag = pFrag->pNext) == NULL) { break; }

      pData = pFrag->pData;
      ismSTORE_getInt(pData, opcount);           // Operations count
      TRACE(9, "Parsing an HA fragment. ChannelId %d, MsgType %u, MsgSqn %lu, " \
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
         TRACE(5, "A store generation (GenId %u, Index %u) has been created on the Standby node. Version %lu, MemSizeBytes %lu, " \
               "GranuleSizeBytes %u, PoolsCount %u, DescriptorStructSize %u, SplitItemStructSize %u, RsrvPoolMemSizeBytes %lu, " \
               "pBaseAddress %p, State %u, Offset 0x%lx, MaxRefsPerGranule %u\n",
               pGenHeader->GenId, genIndex, pGenHeader->Version, pGenHeader->MemSizeBytes,
               pGenHeader->GranulePool[0].GranuleSizeBytes, pGenHeader->PoolsCount,
               pGenHeader->DescriptorStructSize, pGenHeader->SplitItemStructSize,
               pGenHeader->RsrvPoolMemSizeBytes, pGen->pBaseAddress,
               pGenHeader->State, pGen->Offset, pGen->MaxRefsPerGranule);
         break;

      case StoreHAMsg_ActivateGen:
         genIndexPrev = (genIndex + ismStore_memGlobal.InMemGensCount - 1) % ismStore_memGlobal.InMemGensCount;
         pGen = &ismStore_memGlobal.InMemGens[genIndexPrev];
         pGenHeader = (ismStore_memGenHeader_t *)pGen->pBaseAddress;
         TRACE(5, "A store generation (GenId %u, Index %u) has been activated on the Standby node. PrevGenId %u, PrevGenIndex %u, PrevGenState %d\n",
               genId, genIndex, pGenHeader->GenId, genIndexPrev, pGenHeader->State);
         break;

      case StoreHAMsg_AssignRsrvPool:
         TRACE(5, "The store management reserved pool as been assigned on the Standby node\n");
         break;

      case StoreHAMsg_WriteGen:
         pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;
         pGen = &ismStore_memGlobal.InMemGens[genIndex];
         pGenHeader = (ismStore_memGenHeader_t *)pGen->pBaseAddress;

         pGenHeader->State = ismSTORE_GEN_STATE_CLOSE_PENDING;
         ADR_WRITE_BACK(&pGenHeader->State, sizeof(pGenHeader->State));

         // Set the free granules in the generation based on the bitmap
         for (i=0; i < genMap.GranulesMapCount; i++)
         {
            pPool = &pGenHeader->GranulePool[i];
            if ((pBitMap = genMap.GranulesMap[i].pBitMap[ismSTORE_BITMAP_FREE_IDX]) != NULL)
            {
               uint64_t j;
               for (j=0, granulesCount=0; j < genMap.GranulesMap[i].BitMapSize; j++)
               {
                  bitmask = pBitMap[j];
                  for (n=0; bitmask && n < 64; n++, bitmask >>= 1)
                  {
                     if (bitmask & 0x1)
                     {
                        uint64_t off = pPool->Offset + (j * 64 + n) * pPool->GranuleSizeBytes;
                        pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + off);
                        pDescriptor->DataType = ismSTORE_DATATYPE_FREE_GRANULE;
                        granulesCount++;
                     }
                  }
               }
            }
            // Release the bitmap because it is no longer needed
            ismSTORE_FREE(genMap.GranulesMap[i].pBitMap[ismSTORE_BITMAP_FREE_IDX]);
            TRACE(5, "The pool (Index %u) of store generation Id %u (Index %u) has been rearranged. FreeGranulesCount %u, BitMapSize %u\n",
                  i, genId, genIndex, granulesCount, genMap.GranulesMap[i].BitMapSize);
         }

         if ((ec = ism_store_memRecoveryLinkRefChanks(pGenHeader) != ISMRC_OK))
         {
            TRACE(1, "Failed to write generation (GenId %u, Index %u) to the disk due to store recovery error (ism_store_memRecoveryLinkRefChanks). error code %u\n", genId, genIndex, ec);
            rc = StoreRC_SystemError;
            goto exit;
         }

         if ((pDiskWriteCtxt = (ismStore_memHADiskWriteCtxt_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,112),sizeof(*pDiskWriteCtxt))) == NULL)
         {
            TRACE(1, "Failed to write generation (GenId %u, Index %u) to the disk due to memory allocation error\n", genId, genIndex);
            rc = StoreRC_AllocateError;
            goto exit;
         }

         memset(pDiskWriteCtxt, '\0', sizeof(*pDiskWriteCtxt));
         pDiskWriteCtxt->pGen = pGen;
         pDiskWriteCtxt->ViewId = ismStore_memGlobal.HAInfo.View.ViewId;
         pDiskWriteCtxt->pHAChannel = pHAChannel;
         pDiskWriteCtxt->Ack.AckSqn = pHAChannel->pFrag->MsgSqn;
         pDiskWriteCtxt->Ack.FragSqn = pHAChannel->pFrag->FragSqn;
         pDiskWriteCtxt->Ack.SrcMsgType = pHAChannel->pFrag->MsgType;

         // Compact the generation data before writing it to the disk
         memset(&diskTask, '\0', sizeof(diskTask));
         diskTask.GenId = genId;
         diskTask.Priority = 0;
         diskTask.Callback = ism_store_memHADiskWriteComplete;
         diskTask.pContext = pDiskWriteCtxt;

         memset(&genMap, '\0', sizeof(genMap));
         memset(pBitMapsArray, '\0', sizeof(pBitMapsArray));
         if ((ec = ism_store_memCreateGranulesMap(pGenHeader, &genMap, 0)) != ISMRC_OK)
         {
            TRACE(4, "Failed to compact generation Id %u (Index %u), because ism_store_memCreateGranulesMap failed. error code %d\n", genId, genIndex, ec);
            diskTask.BufferParams->pBuffer = (char *)pGenHeader;
            diskTask.BufferParams->BufferLength = pGenHeader->MemSizeBytes;
         }
         else
         {
            for (poolId=0; poolId < genMap.GranulesMapCount; poolId++) { pBitMapsArray[poolId] = genMap.GranulesMap[poolId].pBitMap[ismSTORE_BITMAP_LIVE_IDX]; }
            diskTask.BufferParams->pBitMaps = pBitMapsArray;
            diskTask.BufferParams->fCompactRefChunks = 1;
            if ((ec = ism_storeDisk_compactGenerationData((void *)pGenHeader, diskTask.BufferParams)) == StoreRC_OK)
            {
               compactRatio = 1.0 - (double)diskTask.BufferParams->BufferLength / pGenHeader->MemSizeBytes;
               pDiskWriteCtxt->pCompData = diskTask.BufferParams->pBuffer;
               pDiskWriteCtxt->CompDataSizeBytes = diskTask.BufferParams->BufferLength;
               TRACE(5, "Store generation Id %u (Index %u) has been compacted. RecordsCount %u, CompactSizeBytes %lu, CompactRatio %f, StdDevBytes %lu\n",
                     genId, genIndex, genMap.RecordsCount, diskTask.BufferParams->BufferLength,
                     compactRatio, diskTask.BufferParams->StdDev);
            }
            else
            {
               TRACE(4, "Failed to compact generation Id %u (Index %u), because ism_storeDisk_compactGenerationData failed. error code %d\n", genId, genIndex, ec);
               diskTask.BufferParams->pBuffer = (char *)pGenHeader;
               diskTask.BufferParams->BufferLength = pGenHeader->MemSizeBytes;
            }
            diskTask.BufferParams->pBitMaps = NULL;
         }

         if ((ec = ism_store_memRecoveryAddGeneration(genId, (void *)diskTask.BufferParams->pBuffer, diskTask.BufferParams->BufferLength, 1)) != ISMRC_OK)
         {
            TRACE(1, "Failed to write generation (GenId %u, Index %u) to the disk due to store recovery error (ism_store_memRecoveryAddGeneration). error code %u\n", genId, genIndex, ec);
            ismSTORE_FREE(pDiskWriteCtxt->pCompData);
            ismSTORE_FREE(pDiskWriteCtxt);
            rc = StoreRC_SystemError;
            goto exit;
         }

         pGenHeader->State = ismSTORE_GEN_STATE_WRITE_PENDING;
         ADR_WRITE_BACK(&pGenHeader->State, sizeof(pGenHeader->State));
         TRACE(5, "Store generation (GenId %u, Index %u) is being written to the disk. DiskFileSize %lu\n", genId, genIndex, diskTask.BufferParams->BufferLength);

         if ((rc = ism_storeDisk_writeGeneration(&diskTask)) != StoreRC_OK)
         {
            TRACE(1, "Failed to write store generation (GenId %u, Index %u) to the disk. error code %d\n", genId, genIndex, rc);
            ismSTORE_FREE(pDiskWriteCtxt->pCompData);
            ismSTORE_FREE(pDiskWriteCtxt);
            goto exit;
         }
         break;

      case StoreHAMsg_DeleteGen:
         TRACE(5, "Store generation (GenId %u) is being deleted from the disk\n", genId);
         memset(&diskTask, '\0', sizeof(diskTask));
         diskTask.GenId = genId;
         diskTask.Priority = 0;
         diskTask.Callback = ism_store_memHADiskDeleteComplete;

         if ((rc = ism_storeDisk_deleteGeneration(&diskTask)) != StoreRC_OK)
         {
            TRACE(1, "Failed to delete store generation (GenId %u) from the disk. error code %d\n", genId, rc);
            goto exit;
         }

         if ((ec = ism_store_memRecoveryDelGeneration(genId)) != ISMRC_OK)
         {
            TRACE(1, "Failed to delete store generation (GenId %u) from the recovery memory of the Standby node. error code %d\n", genId, ec);
         }
         break;

      case StoreHAMsg_CompactGen:
         TRACE(5, "Store generation (GenId %u) is being compacted. GranulesMapCount %u, PredictedSizeBytes %lu\n",
               genId, genMap.GranulesMapCount, genMap.PredictedSizeBytes);

         // Allocate an array of pointers to the generation BitMaps. The memory is released by the DiskUtils component
         if ((pBitMaps = (uint64_t **)ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,113),ismSTORE_GRANULE_POOLS_COUNT * sizeof(uint64_t *))) == NULL)
         {
            TRACE(1, "Failed to compact store generation (GenId %u) on the Standby node, due to a memory allocation error\n", genId);
            rc = StoreRC_SystemError;
            goto exit;
         }
         memset(pBitMaps, '\0', ismSTORE_GRANULE_POOLS_COUNT * sizeof(uint64_t *));
         for (i=0; i < genMap.GranulesMapCount; i++)
         {
            if ((pBitMaps[i] = genMap.GranulesMap[i].pBitMap[ismSTORE_BITMAP_LIVE_IDX]) == NULL)
            {
               TRACE(1, "Failed to compact store generation (GenId %u) on the Standby node, because the bitmap (PoolId %u) is NULL. GranulesMapCount %u\n", genId, i, genMap.GranulesMapCount);
               rc = StoreRC_SystemError;
               goto exit;
            }
         }

         ec = ism_store_memRecoveryUpdGeneration(genId, pBitMaps, genMap.PredictedSizeBytes);
         // From this point on the Recovery module is responsible to free the allocated memory
         pBitMaps = NULL;
         memset(&genMap, '\0', sizeof(genMap));

         if (ec != ISMRC_OK)
         {
            TRACE(1, "Failed to compact store generation (GenId %u) on the Standby node. error code %d\n", genId, ec);
            rc = StoreRC_SystemError;
            goto exit;
         }
         break;

      case StoreHAMsg_Shutdown:
         ism_store_memB2H(nodeStr, (uint8_t *)pMgmtHeader->GenToken.NodeId, sizeof(ismStore_HANodeID_t));
         TRACE(5, "An HA controlled termination request has been received. GenToken %s:%lu\n", nodeStr, pMgmtHeader->GenToken.Timestamp);
         rc = StoreRC_HA_StoreTerm;
         break;

      default:
         TRACE(1, "Failed to parse an HA generation message (ChannelId %d, MsgSqn %lu), because the message type %d is not valid\n",
               pHAChannel->ChannelId, pHAChannel->pFrag->MsgSqn, msgType);
         rc = StoreRC_SystemError;
         goto exit;
   }

exit:
   // Release the memory which was allocated for the BitMaps
   if (rc != ISMRC_OK)
   {
      for (poolId=0; poolId < ismSTORE_GRANULE_POOLS_COUNT; poolId++)
      {
         for (i=0; i < ismSTORE_BITMAPS_COUNT; i++)
         {
            ismSTORE_FREE(genMap.GranulesMap[poolId].pBitMap[i]);
         }
      }
      ismSTORE_FREE(pBitMaps);
   }

   // Send back an ACK
   if (pHAChannel->AckingPolicy == 1 && msgType != StoreHAMsg_WriteGen && msgType != StoreHAMsg_CompactGen)
   {
      ack.AckSqn = pHAChannel->pFrag->MsgSqn;
      ack.FragSqn = pHAChannel->pFragTail->FragSqn;
      ack.SrcMsgType = msgType;
      ack.ReturnCode = rc;
      if ((ec = ism_store_memHASendAck(pHAChannel, &ack)) != StoreRC_OK)
      {
         TRACE(1, "Failed to send ACK for HA message (ChannelId %d, MsgType %u, MsgSqn %lu, FragSqn %u, rc %d). error code %d\n",
               pHAChannel->ChannelId, msgType, pHAChannel->pFrag->MsgSqn, pHAChannel->pFragTail->FragSqn, rc, ec);
         if (rc == StoreRC_OK)
         {
            rc = ec;
         }
      }
   }

   return rc;
}

static int ism_store_memHAParseMsg(ismStore_memHAChannel_t *pHAChannel)
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
   ismStore_memHAAck_t ack;
   ismStore_Handle_t handle, ownerHandle, chunkHandle, offset;
   ismStore_GenId_t genId=0;
   uint8_t *pRefState, fNewChunk, flags=0;
   uint32_t opLength, opcount, mutexIndex;
   uint64_t baseOrderId;
   char *pData, *pBase;
   int rc = StoreRC_OK, ec, i;

   pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;
   memset(&ack, '\0', sizeof(ack));

   TRACE(9, "Parsing an HA message. ChannelId %d, MsgType %u, MsgSqn %lu, LastFrag %u\n",
         pHAChannel->ChannelId, pHAChannel->pFrag->MsgType, pHAChannel->pFrag->MsgSqn, pHAChannel->pFragTail->FragSqn);

   for (pFrag = pHAChannel->pFrag; pFrag; pFrag = pFrag->pNext)
   {
      pData = pFrag->pData;
      ismSTORE_getInt(pData, opcount);           // Operations count

      TRACE(9, "Parsing an HA fragment. ChannelId %d, MsgType %u, MsgSqn %lu, " \
            "FragSqn %u, IsLastFrag %u, DataLength %lu, OpCount %u\n",
            pHAChannel->ChannelId, pFrag->MsgType, pFrag->MsgSqn,
            pFrag->FragSqn, isLastFrag(pFrag->flags), pFrag->DataLength, opcount);
      flags = pFrag->flags ; 

      for (i=0; i < opcount; i++)
      {
         pBase = pData;
         ismSTORE_getShort(pData, opType);       // Operation type
         ismSTORE_getInt(pData, opLength);       // Operation data length
         TRACE(9, "Parsing an HA operation ChannelId %d, opType %u, opLength %u\n",pHAChannel->ChannelId,opType,opLength);

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
               ismSTORE_getLong(pData, pRef->RefHandle);
               ismSTORE_getInt(pData, pRef->Value);
               ismSTORE_getChar(pData, pRef->State);

               if (fNewChunk) {
                  ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor) + pDescriptor->DataLength);
               } else {
                  ADR_WRITE_BACK(pRef, sizeof(*pRef));
               }
               break;

            case Operation_DeleteReference:
               ismSTORE_getLong(pData, handle);

               pRef = (ismStore_memReference_t *)ism_store_memMapHandleToAddress(handle);
               memset(pRef, '\0', sizeof(ismStore_memReference_t));
               ADR_WRITE_BACK(pRef, sizeof(*pRef));
               break;

            case Operation_UpdateReference:
               ismSTORE_getLong(pData, handle);

               pRef = (ismStore_memReference_t *)ism_store_memMapHandleToAddress(handle);
               ismSTORE_getChar(pData, pRef->State);
               ADR_WRITE_BACK(&pRef->State, sizeof(pRef->State));
               break;

            case Operation_CreateRecord:
               ismSTORE_getLong(pData, handle);
               pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(handle);
               memcpy(pDescriptor, pData, opLength - LONG_SIZE);
               ADR_WRITE_BACK(pDescriptor, opLength - LONG_SIZE);
               pData += (opLength - LONG_SIZE);
               break;

            case Operation_DeleteRecord:
               ismSTORE_getLong(pData, handle);
               pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(handle);

               pDescriptor->DataType = ismSTORE_DATATYPE_FREE_GRANULE;
               pDescriptor->NextHandle = ismSTORE_NULL_HANDLE;
               pDescriptor->DataLength = 0;
               pDescriptor->TotalLength = 0;
               pDescriptor->Attribute = 0;
               pDescriptor->State = 0;
               ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor));
               break;

            case Operation_UpdateRecord:
               ismSTORE_getLong(pData, handle);
               pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(handle);

               ismSTORE_getLong(pData, pDescriptor->Attribute);
               ismSTORE_getLong(pData, pDescriptor->State);
               ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor));
               break;

            case Operation_UpdateRecordAttr:
               ismSTORE_getLong(pData, handle);
               pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(handle);

               ismSTORE_getLong(pData, pDescriptor->Attribute);
               ADR_WRITE_BACK(&pDescriptor->Attribute, sizeof(pDescriptor->Attribute));
               break;

            case Operation_UpdateRecordState:
               ismSTORE_getLong(pData, handle);
               pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(handle);

               ismSTORE_getLong(pData, pDescriptor->State);
               ADR_WRITE_BACK(&pDescriptor->State, sizeof(pDescriptor->State));
               break;

            case Operation_UpdateRefState:
               ismSTORE_getLong(pData, handle);
               ismSTORE_getLong(pData, ownerHandle);
               ismSTORE_getLong(pData, baseOrderId);

               genId = ismSTORE_EXTRACT_GENID(handle);
               offset = ismSTORE_EXTRACT_OFFSET(handle);
               chunkHandle = ismSTORE_BUILD_HANDLE(genId, ismSTORE_ROUND(offset, pMgmtHeader->GranulePool[ismSTORE_MGMT_SMALL_POOL_INDEX].GranuleSizeBytes));
               pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(chunkHandle);
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
               ismSTORE_getChar(pData, *pRefState);

               if (fNewChunk) {
                  ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor) + pDescriptor->DataLength);
               } else {
                  ADR_WRITE_BACK(pRefState, 1);
               }
               break;

            case Operation_CreateState:
               ismSTORE_getLong(pData, handle);
               ismSTORE_getLong(pData, ownerHandle);

               genId = ismSTORE_EXTRACT_GENID(handle);
               offset = ismSTORE_EXTRACT_OFFSET(handle);
               chunkHandle = ismSTORE_BUILD_HANDLE(genId, ismSTORE_ROUND(offset, ismStore_memGlobal.MgmtGranuleSizeBytes));
               pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(chunkHandle);
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
               ismSTORE_getInt(pData, pStateObject->Value);
               ismSTORE_getChar(pData, pStateObject->Flag);
               
               TRACE(9, "A state object (Handle 0x%lx, Value %d, Flag %u, ChunkHandle 0x%lx, fNewChunk %u) for owner " \
                     "(Handle 0x%lx, DataType 0x%x, Version %u) has been added on the Standby node\n",
                     handle, pStateObject->Value, pStateObject->Flag, chunkHandle, fNewChunk,
                     ownerHandle, pOwnerDescriptor->DataType, pSplit->Version);

               if (fNewChunk) {
                  ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor) + pDescriptor->DataLength);
               } else {
                  ADR_WRITE_BACK(pStateObject, sizeof(*pStateObject));
               }
               break;

            case Operation_DeleteState:
               ismSTORE_getLong(pData, handle);
               pStateObject = (ismStore_memState_t *)ism_store_memMapHandleToAddress(handle);
               memset(pStateObject, '\0', sizeof(ismStore_memState_t));
               ADR_WRITE_BACK(pStateObject, sizeof(*pStateObject));
               break;

            case Operation_UpdateActiveOid:
               ismSTORE_getLong(pData, handle);
               pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(handle);
               memcpy(pDescriptor, pData, opLength - LONG_SIZE);
               pData += (opLength - LONG_SIZE);
               ADR_WRITE_BACK(pDescriptor, opLength - LONG_SIZE);
               break;

            default:
               TRACE(1, "Failed to parse a store operation in HA fragment, because the operation type %d is not valid. ChannelId %d, MsgType %u, MsgSqn %lu, FragSqn %u, IsLastFrag %u, DataLength %lu, OpCount %u, OpDataLength %u\n",
                     opType, pHAChannel->ChannelId, pFrag->MsgType, pFrag->MsgSqn, pFrag->FragSqn, isLastFrag(pFrag->flags), pFrag->DataLength, opcount, opLength);
               rc = StoreRC_SystemError;
               goto exit;
         }
         pData = pBase + (SHORT_SIZE + INT_SIZE + opLength);
      }
   }

exit:
   // Send back an ACK
   if (pHAChannel->AckingPolicy == 1 && pHAChannel->pFrag->MsgType != StoreHAMsg_UpdateActiveOid && !isNoAck(flags) )
   {
      ack.AckSqn = pHAChannel->pFrag->MsgSqn;
      ack.FragSqn = pHAChannel->pFragTail->FragSqn;
      ack.SrcMsgType = pHAChannel->pFrag->MsgType;
      ack.ReturnCode = rc;
      if ((ec = ism_store_memHASendAck(pHAChannel, &ack)) != StoreRC_OK)
      {
         TRACE(1, "Failed to send ACK for HA message (ChannelId %d, MsgType %u, MsgSqn %lu, FragSqn %u, rc %d). error code %d\n",
               pHAChannel->ChannelId, pHAChannel->pFrag->MsgType, pHAChannel->pFrag->MsgSqn, pHAChannel->pFragTail->FragSqn, rc, ec);
         if (rc == StoreRC_OK)
         {
            rc = ec;
         }
      }
   }

   return rc;
}

/*********************************************************************/
/* Store High-Availability Operations for New Node Synchronization   */
/*********************************************************************/
int ism_store_memHASyncStart(void)
{
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   ismStore_memHAChannel_t *pSyncChannel=NULL, *pAdminChannel=NULL;
   ismStore_memHAJob_t job;
   int rc = StoreRC_OK, ec;

   TRACE(9, "Entry: %s. fActive %u\n", __FUNCTION__, ismStore_memGlobal.fActive);
   pthread_mutex_lock(&pHAInfo->Mutex);

   // Make sure that previous HASync thread terminated before we start a new synchronization procedure
   pHAInfo->fThreadGoOn = 0;
   while (pHAInfo->fThreadUp)   /* BEAM suppression: infinite loop */
   {
      pthread_cond_signal(&pHAInfo->ThreadCond);
      pthread_mutex_unlock(&pHAInfo->Mutex);
      ism_common_sleep(1000);
      pthread_mutex_lock(&pHAInfo->Mutex);
   }

   pHAInfo->SyncState = 0x1;
   pHAInfo->SyncRC = ISMRC_OK;
   pHAInfo->fSyncLocked = 0;
   pHAInfo->SyncCurMemSizeBytes = 0;
   pHAInfo->SyncSentBytes = 0;
   pHAInfo->SyncExpGensCount = pHAInfo->SyncSentGensCount = 0;
   memset(pHAInfo->SyncTime, '\0', sizeof(pHAInfo->SyncTime));
   pHAInfo->SyncTime[0] = ism_common_currentTimeNanos();

   // Create an HA channel for the sync procedure
   if ((rc = ism_store_memHACreateChannel(ismSTORE_HA_CHID_SYNC, 0x4, &pSyncChannel)) != StoreRC_OK)
   {
      goto exit;
   }

   // Create an HA channel for the Admin layer
   if ((rc = ism_store_memHACreateChannel(ismSTORE_HA_CHID_ADMIN, 0, &pAdminChannel)) != StoreRC_OK)
   {
      goto exit;
   }

   pHAInfo->pSyncChannel = pSyncChannel;
   pHAInfo->pAdminChannel = pAdminChannel;
   pHAInfo->fAdminTx = 1;  // Mark the Admin channel as transmitter

   // Create the HASync thread
   pHAInfo->fThreadGoOn = 1;
   if (ism_common_startThread(&pHAInfo->ThreadId, ism_store_memHASyncThread, NULL,
       NULL, 0, ISM_TUSAGE_NORMAL, 0, ismSTORE_HA_SYNC_THREAD_NAME, "Store HA node synchronization"))
   {
      TRACE(1, "HASync: Failed to create the %s thread - errno %d.\n", ismSTORE_HA_SYNC_THREAD_NAME, errno);
      rc = StoreRC_SystemError;
      goto exit;
   }

   memset(&job, '\0', sizeof(job));
   job.JobType = StoreHAJob_SyncList;
   TRACE(5, "HASync: The Store new node synchronization is being started. SyncState 0x%x\n", pHAInfo->SyncState);
   pthread_mutex_unlock(&pHAInfo->Mutex);

   TRACE(5, "HASync: Calling ism_ha_admin_transfer_state API\n");
   // Inform the Admin layer that a new node synchronization has been started
   if ((ec = ism_ha_admin_transfer_state(pHAInfo->View.autoHAGroup)) != ISMRC_OK)
   {
      TRACE(1, "HASync: The ism_ha_admin_transfer_state API failed. error code %d\n", ec);
      rc = StoreRC_SystemError;
      goto exit;
   }

   ism_store_memHAAddJob(&job);

   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
   return rc;

exit:
   if (rc != StoreRC_OK)
   {
      if (pSyncChannel)
      {
         ism_store_memHACloseChannel(pSyncChannel, 1);
         pHAInfo->pSyncChannel = pSyncChannel = NULL;
      }
      if (pAdminChannel)
      {
         ism_store_memHACloseChannel(pAdminChannel, 1);
         pHAInfo->pAdminChannel = pAdminChannel = NULL;
         pHAInfo->fAdminTx = 0;  // Mark the Admin channel as receiver
      }
      pHAInfo->SyncRC = ISMRC_StoreHAError;
      pHAInfo->fThreadGoOn = 0;
      pthread_cond_signal(&pHAInfo->ThreadCond);
   }
   pthread_mutex_unlock(&pHAInfo->Mutex);

   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
   return rc;
}

int ism_store_memHASyncWaitView(void)
{
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   int rc;

   TRACE(5, "Entry: %s. ActiveNodesCount %u, SyncNodesCount %u, State %u\n",
         __FUNCTION__, pHAInfo->AdminView.ActiveNodesCount, pHAInfo->AdminView.SyncNodesCount, pHAInfo->State);

   pthread_mutex_lock(&pHAInfo->Mutex);
   while (pHAInfo->AdminView.SyncNodesCount < 2 &&   /* BEAM suppression: infinite loop */
          pHAInfo->State >= ismSTORE_HA_STATE_UNSYNC &&
          pHAInfo->State <= ismSTORE_HA_STATE_PRIMARY)
   {
      pthread_cond_wait(&pHAInfo->ViewCond, &pHAInfo->Mutex);
   }
   rc = (pHAInfo->AdminView.SyncNodesCount == 2 ? StoreRC_OK : StoreRC_SystemError);
   pthread_mutex_unlock(&pHAInfo->Mutex);

   TRACE(5, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

int ism_store_memHASyncWaitDisk(void)
{
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   ism_time_t timeout;
   struct timespec abstime;
   int rc;

   TRACE(5, "Entry: %s. ActiveNodesCount %u, SyncNodesCount %u, State %u, SyncMemSizeBytes %lu:%lu, SyncRC %d\n",
         __FUNCTION__, pHAInfo->View.ActiveNodesCount, pHAInfo->SyncNodesCount, pHAInfo->State, pHAInfo->SyncCurMemSizeBytes, pHAInfo->SyncMaxMemSizeBytes, pHAInfo->SyncRC);

   timeout = ism_common_monotonicTimeNanos() + 600000000000;
   abstime.tv_sec = (time_t)(timeout / 1000000000);
   abstime.tv_nsec = (long)(timeout % 1000000000);

   pthread_mutex_lock(&pHAInfo->Mutex);
   while (pHAInfo->SyncRC == ISMRC_OK &&   /* BEAM suppression: infinite loop */
          pHAInfo->SyncCurMemSizeBytes > 0 &&
          ism_common_monotonicTimeNanos() < timeout)
   {
      TRACE(9, "HASync: waits for Standby disk write. SyncMemSizeBytes %lu:%lu\n",
            pHAInfo->SyncCurMemSizeBytes, pHAInfo->SyncMaxMemSizeBytes);
      ism_common_cond_timedwait(&pHAInfo->SyncCond, &pHAInfo->Mutex, &abstime, 0);
   }

   rc = (pHAInfo->SyncCurMemSizeBytes == 0 && pHAInfo->SyncRC == ISMRC_OK ? StoreRC_OK : StoreRC_SystemError);
   pthread_mutex_unlock(&pHAInfo->Mutex);

   TRACE(5, "Exit: %s. SyncMemSizeBytes %lu, SyncRC %d, rc %d\n", __FUNCTION__, pHAInfo->SyncCurMemSizeBytes, pHAInfo->SyncRC, rc);

   return rc;
}

int8_t ism_store_memHAGetSyncCompPct(void)
{
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   int8_t rc=-1;
   double pct=0;

   pthread_mutex_lock(&pHAInfo->Mutex);
   if (pHAInfo->SyncState > 0)
   {
      if (pHAInfo->SyncExpGensCount > 0)
      {
         pct = 100 * ((double)pHAInfo->SyncSentGensCount / pHAInfo->SyncExpGensCount);
         if (pct >= 100) { pct = 99; }
      }
      if (pct == 0) { pct = 1; }
      rc = (int8_t)pct;
   }
   pthread_mutex_unlock(&pHAInfo->Mutex);
   return rc;
}

static int ism_store_memHASyncComplete(void)
{
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   ismStore_memStream_t *pStream;
   ismStore_memGenMap_t *pGenMap;
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memJob_t job;
   ismStore_memHAMsgType_t msgType = StoreHAMsg_SyncComplete;
   ismStore_memHAAck_t ack;
   ism_time_t currtime;
   double syncTime, lockedTime, adminTime, sbTime;
   char *pBuffer=NULL, *pPos;
   uint8_t flags;
   uint32_t bufferLength, opcount;
   int rc = StoreRC_OK, i;

   TRACE(9, "Entry: %s. SyncState 0x%x, State %u, ActiveNodesCount %u\n", __FUNCTION__, pHAInfo->SyncState, pHAInfo->State, pHAInfo->View.ActiveNodesCount);
   pthread_mutex_lock(&pHAInfo->Mutex);

   // See whether the Admin completes to transfer its state
   if (!(pHAInfo->SyncState & 0xc0))
   {
      TRACE(5, "HASync: The Admin didn't complete to transfer its state to the Standby node yet. SyncState 0x%x\n", pHAInfo->SyncState);
      // Wait until the Admin completes to transfer its state
      while (!(pHAInfo->SyncState & 0xc0) &&   /* BEAM suppression: infinite loop */
           pHAInfo->State == ismSTORE_HA_STATE_PRIMARY &&
             pHAInfo->View.ActiveNodesCount > 1)
      {
         pthread_cond_wait(&pHAInfo->SyncCond, &pHAInfo->Mutex);
      }
   }
   pHAInfo->SyncTime[4] = ism_common_currentTimeNanos();

   if (pHAInfo->State != ismSTORE_HA_STATE_PRIMARY)
   {
      pHAInfo->SyncNodesCount = 0;
      rc = StoreRC_HA_ConnectionBroke;
   }
   else if (pHAInfo->View.ActiveNodesCount < 2)
   {
      pHAInfo->SyncNodesCount = 1;
      rc = StoreRC_HA_ConnectionBroke;
   }
   else if (pHAInfo->SyncState & 0x80)
   {
      rc = StoreRC_SystemError;
   }
   else
   {
      // Send a SyncComplete message
      if ((rc = ism_store_memHAEnsureBufferAllocation(pHAInfo->pSyncChannel, &pBuffer, &bufferLength, &pPos, 64, msgType, &opcount)) == StoreRC_OK)
      {
         // Send the last fragment
         // requiredLength = 0 means that this is the last fragment.
         memset(&ack, '\0', sizeof(ack));
         ack.AckSqn = pHAInfo->pSyncChannel->MsgSqn;
         if ((rc = ism_store_memHAEnsureBufferAllocation(pHAInfo->pSyncChannel, &pBuffer, &bufferLength, &pPos, 0, msgType, &opcount)) == StoreRC_OK)
         {
            if ((rc = ism_store_memHAReceiveAck(pHAInfo->pSyncChannel, &ack, 0)) == StoreRC_OK)
            {
               pHAInfo->View.OldRole = pHAInfo->View.NewRole;
            }
            else
            {
               TRACE(1, "HASync: Failed to receive an ACK from the Standby node. MsgType %u, MsgSqn %lu, LastFrag %u, error code %d\n",
                     msgType, pHAInfo->pSyncChannel->MsgSqn, pHAInfo->pSyncChannel->FragSqn, rc);
            }
         }
         else
         {
            TRACE(1, "HASync: Failed to send a message (MsgType %u, MsgSqn %lu, LastFrag %u) to the Standby node. error code %d\n",
                  msgType, pHAInfo->pSyncChannel->MsgSqn, pHAInfo->pSyncChannel->FragSqn, rc);
         }
      }
      else
      {
         TRACE(1, "HASync: Failed to send a message (MsgType %u, MsgSqn %lu, LastFrag %u) to the Standby node. error code %d\n",
               msgType, pHAInfo->pSyncChannel->MsgSqn, pHAInfo->pSyncChannel->FragSqn, rc);
      }
   }

   pHAInfo->fThreadGoOn = 0;
   pthread_mutex_unlock(&pHAInfo->Mutex);

   if (rc == StoreRC_OK)
   {
      pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);

      if ( ismStore_memGlobal.fEnablePersist )
      {
        ism_storePersist_flushQ(0,-1) ; 
      }

      for (i=0; i < ismStore_memGlobal.GensMapSize; i++)
      {
         if ((pGenMap = ismStore_memGlobal.pGensMap[i]) != NULL)
         {
            TRACE(7, "HASync: Generation (GenId %u) has been synchronized. DiskFileSize %lu, HASyncState %u\n",
                  i, pGenMap->DiskFileSize, pGenMap->HASyncState);
            if (pGenMap->HASyncState != StoreHASyncGen_Sent && pGenMap->HASyncState != StoreHASyncGen_Proposed && pGenMap->DiskFileSize > 0)
            {
               TRACE(1, "HASync: The generation synchronization state is not valid. GenId %u, DiskFileSize %lu, HASyncState %u\n",
                     i, pGenMap->DiskFileSize, pGenMap->HASyncState);
            }
         }
      }

      currtime = ism_common_currentTimeNanos();
      syncTime = (double)(currtime - pHAInfo->SyncTime[0]) / 1e9;
      lockedTime = (double)(currtime - pHAInfo->SyncTime[3]) / 1e9;
      adminTime = (double)(pHAInfo->SyncTime[1] - pHAInfo->SyncTime[0]) / 1e9;
      sbTime = (double)(currtime - pHAInfo->SyncTime[4]) / 1e9;
      TRACE(5, "HASync: Synchronization summary. GensCount %u, SyncSentGensCount %u, SyncExpGensCount %u, SyncSentBytes %lu, SyncTime %.6f (lockedTime %.6f, adminTime %.6f, sbExTime %.6f)\n",
            (ismStore_memGlobal.GensMapCount - 1), pHAInfo->SyncSentGensCount, pHAInfo->SyncExpGensCount, pHAInfo->SyncSentBytes, syncTime, lockedTime, adminTime, sbTime);

      // Opens an HA channel for each active stream
      for (i=0; i < ismStore_memGlobal.StreamsSize; i++)
      {
         if ((pStream = ismStore_memGlobal.pStreams[i]))
         {
            flags=0;
            if (pStream->fHighPerf) { flags |= 0x1; }
            if (i == 0) { flags |= 0x2; }

            TRACE(7, "HASync: Creating an HA channel for active stream. Index %u, MyGenId %u, ActiveGenId %u, Flags 0x%x\n",
                  i, pStream->MyGenId, pStream->ActiveGenId, flags);
            if ((rc = ism_store_memHACreateChannel(i, flags, &pStream->pHAChannel)) != StoreRC_OK)
            {
               TRACE(1, "HASync: Could not complete the new node synchronization procedure " \
                     "due to a channel creation failure. ChannelId %u, error code %d\n", i, rc);
               break;
            }
         }
      }
      pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
   }

   if (rc == StoreRC_OK)
   {
      pthread_mutex_lock(&pHAInfo->Mutex);
      pHAInfo->SyncNodesCount++;
      ism_store_memHASetHasStandby() ; 
      pthread_mutex_unlock(&pHAInfo->Mutex);
      // Set HaveData since it is now also set on the Standby 
      pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;
      if (pMgmtHeader->HaveData == 0)
      {
        pMgmtHeader->HaveData = 1;
        ADR_WRITE_BACK(&pMgmtHeader->HaveData, 1); 
      }
      TRACE(5, "HASync: The new node synchronization procedure has been completed successfully\n");
      memset(&job, '\0', sizeof(job));
      job.JobType = StoreJob_HAViewChanged;
      job.HAView.ViewId = pHAInfo->View.ViewId;
      job.HAView.NewRole = pHAInfo->View.NewRole;
      job.HAView.OldRole = pHAInfo->View.OldRole;
      job.HAView.ActiveNodesCount = pHAInfo->View.ActiveNodesCount;
      ism_store_memAddJob(&job);
   }
   else
   {
      TRACE(1, "HASync: Failed to synchronize the Standby node into the HA pair. error code %d\n", rc);
   }

   TRACE(9, "Exit: %s. SyncState 0x%x, State %u, SyncNodesCount %u, rc %d\n", __FUNCTION__, pHAInfo->SyncState, pHAInfo->State, pHAInfo->SyncNodesCount, rc);
   return rc;
}

static int ism_store_memHASyncSendList(void)
{
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_memGenIdChunk_t *pGenIdChunk;
   ismStore_memGenMap_t *pGenMap;
   ismStore_memOperationType_t opType;
   ismStore_Handle_t handle;
   ismStore_GenId_t genId;
   ismStore_memHAMsgType_t msgType = StoreHAMsg_SyncList;
   ismStore_memHAJob_t job;
   uint8_t flags, fLastFrag=0;
   uint32_t syncPropGensCount=0, opcount, fragSqn, fragLength, bufferLength, opLength;
   uint64_t msgSqn;
   char *pBuffer=NULL, *pPos, nodeStr[40];
   int i, rc = StoreRC_OK, ec;

   // Create a list of disk generations to be sent to the Standby node
   TRACE(9, "Entry: %s\n", __FUNCTION__);
   pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);

   pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;
   handle = pMgmtHeader->GenIdHandle;
   while (handle)
   {
      pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(handle);
      pGenIdChunk = (ismStore_memGenIdChunk_t *)(pDescriptor + 1);
      for (i=0; i < pGenIdChunk->GenIdCount; i++)
      {
         genId = pGenIdChunk->GenIds[i];
         if ((pGenMap = ismStore_memGlobal.pGensMap[genId]))
         {
            pGenMap->HASyncState = StoreHASyncGen_Empty;
            pGenMap->pHASyncBuffer = NULL;
            pGenMap->HASyncBufferLength = 0;
            pGenMap->HASyncDataLength = 0;
            if (pGenMap->DiskFileSize > 0 || genId == ismSTORE_MGMT_GEN_ID)
            {
               if ((rc = ism_store_memHAEnsureBufferAllocation(pHAInfo->pSyncChannel,
                                                               &pBuffer,
                                                               &bufferLength,
                                                               &pPos,
                                                               64,
                                                               msgType,
                                                               &opcount)) != StoreRC_OK)
               {
                  TRACE(1, "HASyncList: Failed to send a message (MsgType %u, MsgSqn %lu, LastFrag %u) to the Standby node. error code %d\n",
                        msgType, pHAInfo->pSyncChannel->MsgSqn, pHAInfo->pSyncChannel->FragSqn, rc);
                  pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
                  goto exit;
               }

               ismSTORE_putShort(pPos, Operation_Null);
               ismSTORE_putInt(pPos, SHORT_SIZE + sizeof(ismStore_memGenToken_t) + LONG_SIZE);
               ismSTORE_putShort(pPos, genId);
               memcpy(pPos, &pGenMap->GenToken, sizeof(ismStore_memGenToken_t));
               pPos += sizeof(ismStore_memGenToken_t);
               ismSTORE_putLong(pPos, pGenMap->DiskFileSize);
               opcount++;
               pGenMap->HASyncState = StoreHASyncGen_Proposed;
               syncPropGensCount++;

               ism_store_memB2H(nodeStr, (uint8_t *)pGenMap->GenToken.NodeId, sizeof(ismStore_HANodeID_t));
               TRACE(7, "HASyncList: Adding GenId %u, GenToken %s:%lu, DiskFileSize %lu\n",
                     genId, nodeStr, pGenMap->GenToken.Timestamp, pGenMap->DiskFileSize);
            }
         }
      }
      handle = pDescriptor->NextHandle;
   }
   pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);

   // Send the last fragment
   // requiredLength = 0 means that this is the last fragment.
   if ((rc = ism_store_memHAEnsureBufferAllocation(pHAInfo->pSyncChannel, &pBuffer, &bufferLength, &pPos, 0, msgType, &opcount)) != StoreRC_OK)
   {
      TRACE(1, "HASyncList: Failed to send a message (MsgType %u, MsgSqn %lu, LastFrag %u) to the Standby node. error code %d\n",
            msgType, pHAInfo->pSyncChannel->MsgSqn, pHAInfo->pSyncChannel->FragSqn, rc);
      goto exit;
   }

   TRACE(5, "HASyncList: A message (ChannelId %d, MsgType %u, MsgSqn %lu, LastFrag %u, AckSqn %lu) has been sent. SyncPropGensCount %u\n",
         pHAInfo->pSyncChannel->ChannelId, msgType, pHAInfo->pSyncChannel->MsgSqn - 1,
         pHAInfo->pSyncChannel->FragSqn, pHAInfo->pSyncChannel->AckSqn, syncPropGensCount);

   // Wait for the Standby response, which contains the list of required generations
   while (!fLastFrag)
   {
      if ((rc = ism_storeHA_receiveMessage(pHAInfo->pSyncChannel->hChannel, &pBuffer, &bufferLength, 0)) != StoreRC_OK)
      {
         TRACE(1, "HASyncList: Failed to receive a response from the Standby node. error code %d\n", rc);
         goto exit;
      }

      pPos = pBuffer;
      ismSTORE_getInt(pPos, fragLength);            // Fragment length
      ismSTORE_getShort(pPos, msgType);             // Message type
      if (msgType != StoreHAMsg_SyncListRes)
      {
         TRACE(1, "HASyncList: Failed to parse the response from the Standby node, because the message type (%d) is not valid\n", msgType);
         ism_storeHA_returnBuffer(pHAInfo->pSyncChannel->hChannel, pBuffer);
         rc = StoreRC_SystemError;
         goto exit;
      }

      ismSTORE_getLong(pPos, msgSqn);               // Message sequence
      ismSTORE_getInt(pPos, fragSqn);               // Fragment sequence
      ismSTORE_getChar(pPos, flags);                // Fragment flags
      fLastFrag = isLastFrag(flags);                // isLastFragment
      pPos += INT_SIZE;                             // Reserved
      ismSTORE_getInt(pPos, opcount);               // Operations count

      TRACE(7, "HASyncList: %d generations were requested by the Standby node. FragLength %u, MsgType %u, MsgSqn %lu, FragSqn %u, fLastFrag %u\n",
            opcount, fragLength, msgType, msgSqn, fragSqn, fLastFrag);

      pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);
      while (opcount--)
      {
         ismSTORE_getShort(pPos, opType);
         ismSTORE_getInt(pPos, opLength);
         if (opType != Operation_Null || opLength != SHORT_SIZE)
         {
            TRACE(1, "HASyncList: Failed to parse the response from the Standby node, because the message header is not valid. opType %d, opLength %u\n", opType, opLength);
            pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
            ism_storeHA_returnBuffer(pHAInfo->pSyncChannel->hChannel, pBuffer);
            rc = StoreRC_SystemError;
            goto exit;
         }

         ismSTORE_getShort(pPos, genId);
         if ((pGenMap = ismStore_memGlobal.pGensMap[genId]) == NULL)
         {
            TRACE(1, "HASyncList: The generation file (GenId %u) that was requested by the Standby node is not valid\n", genId);
            pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
            ism_storeHA_returnBuffer(pHAInfo->pSyncChannel->hChannel, pBuffer);
            rc = StoreRC_SystemError;
            goto exit;
         }
         pGenMap->HASyncState = StoreHASyncGen_Requested;

         if (genId == ismSTORE_MGMT_GEN_ID)
         {
            pHAInfo->SyncExpGensCount++;
            TRACE(7, "HASyncList: The management generation (GenId %u) has been requested by the Standby node\n", genId);
            continue;
         }

         if (pGenMap->DiskFileSize == 0)
         {
            pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
            TRACE(1, "HASyncList: Could not find a generation file (GenId %u) on the primary disk\n", genId);
            ism_storeHA_returnBuffer(pHAInfo->pSyncChannel->hChannel, pBuffer);
            rc = StoreRC_SystemError;
            goto exit;
         }

         pHAInfo->SyncExpGensCount++;
         TRACE(7, "HASyncList: A generation file (GenId %u, DiskFileSize %lu) has been requested by the Standby node. SyncExpGensCount %u\n",
               genId, pGenMap->DiskFileSize, pHAInfo->SyncExpGensCount);
      }
      pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);

      if ((ec = ism_storeHA_returnBuffer(pHAInfo->pSyncChannel->hChannel, pBuffer)) != StoreRC_OK)
      {
         TRACE(1, "HASyncList: Failed to return a buffer (ChannelId %d, MsgType %u, MsgSqn %lu, FragSqn %u). error code %d\n",
               pHAInfo->pSyncChannel->ChannelId, msgType, msgSqn, fragSqn, ec);
      }
   }

   if (pHAInfo->SyncExpGensCount == 0)
   {
      pthread_mutex_lock(&pHAInfo->Mutex);
      pHAInfo->SyncState |= 0xe;
      TRACE(5, "HASyncList: No need to send any generations to the Standby node. SyncState 0x%x\n", pHAInfo->SyncState);

      memset(&job, '\0', sizeof(job));
      job.JobType = StoreHAJob_SyncComplete;
      ism_store_memHAAddJob(&job);
      pthread_mutex_unlock(&pHAInfo->Mutex);
      rc = StoreRC_OK;
      goto exit;
   }

   TRACE(5, "HASyncList: %u generations have been requested by the Standby node. SyncPropGensCount %u\n", pHAInfo->SyncExpGensCount, syncPropGensCount);
   rc = ism_store_memHASyncReadDiskGen();

exit:
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
   return rc;
}

static int ism_store_memHASyncReadDiskGen(void)
{
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   ismStore_memHAJob_t job;
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_memGenIdChunk_t *pGenIdChunk;
   ismStore_memGenMap_t *pGenMap;
   ismStore_GenId_t genId;
   ismStore_Handle_t handle;
   ismStore_DiskBufferParams_t buffParams;
   ismStore_DiskTaskParams_t diskTask;
   ism_time_t elapsedTime;
   uint8_t fGenFound;
   uint64_t diskFileSize;
   int rc = StoreRC_OK, ec, i;

   TRACE(9, "Entry: %s\n", __FUNCTION__);
   pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;

   while (1)
   {
      ism_common_going2work();
      pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);
      handle = pMgmtHeader->GenIdHandle;
      fGenFound = 0;
      while (handle && !fGenFound)
      {
         pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(handle);
         pGenIdChunk = (ismStore_memGenIdChunk_t *)(pDescriptor + 1);
         for (i=0; i < pGenIdChunk->GenIdCount; i++)
         {
            genId = pGenIdChunk->GenIds[i];
            if ((pGenMap = ismStore_memGlobal.pGensMap[genId]) == NULL)
            {
               TRACE(1, "There is no GenMap entry for generation Id %u in the Standby\n", genId);
               pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
               return StoreRC_SystemError;
            }

            diskFileSize = pGenMap->DiskFileSize;
            if (diskFileSize > 0 && (pGenMap->HASyncState == StoreHASyncGen_Requested || (pGenMap->HASyncState == StoreHASyncGen_Empty && (pHAInfo->SyncState & 0x2))))
            {
               // We have found a generation file to be sent
               if (pGenMap->HASyncState == StoreHASyncGen_Empty)
               {
                  pHAInfo->SyncExpGensCount++;
                  pGenMap->HASyncState = StoreHASyncGen_Requested;
               }
               fGenFound = 1;
               break;
            }
         }
         handle = pDescriptor->NextHandle;
      }
      pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
      pthread_mutex_lock(&pHAInfo->Mutex);

      if (!fGenFound)
      {
         TRACE(7, "HASync: There are no more generation files to read from the primary disk. fSyncLocked %d, SyncCureMemSizeBytes %lu\n",
               pHAInfo->fSyncLocked, pHAInfo->SyncCurMemSizeBytes);

         // See whether all the generation files have been sent
         if (pHAInfo->SyncCurMemSizeBytes > 0)
         {
            pthread_mutex_unlock(&pHAInfo->Mutex);
            break;
         }

         // See whether the Admin completes to transfer its state
         if (!(pHAInfo->SyncState & 0xc0))
         {
            TRACE(5, "HASync: The Admin didn't complete to transfer its state to the Standby node yet. SyncState 0x%x\n", pHAInfo->SyncState);
            // Wait until the Admin completes to transfer its state
            while (!(pHAInfo->SyncState & 0xc0) &&   /* BEAM suppression: infinite loop */
                   pHAInfo->State == ismSTORE_HA_STATE_PRIMARY &&
                   pHAInfo->View.ActiveNodesCount > 1)
            {
               pthread_cond_wait(&pHAInfo->SyncCond, &pHAInfo->Mutex);
            }

            if (pHAInfo->State != ismSTORE_HA_STATE_PRIMARY || pHAInfo->View.ActiveNodesCount < 2)
            {
               rc = StoreRC_HA_ConnectionBroke;
               pthread_mutex_unlock(&pHAInfo->Mutex);
               break;
            }

            if ((pHAInfo->SyncState & 0x80) || !(pHAInfo->SyncState & 0x40))
            {
               rc = StoreRC_SystemError;
               pthread_mutex_unlock(&pHAInfo->Mutex);
               break;
            }
            TRACE(5, "HASync: The Admin completed to transfer its state to the Standby node. SyncState 0x%x\n", pHAInfo->SyncState);

         }

         if (!(pHAInfo->SyncState & 0x2))
         {
            // We are going to do another iteration over the disk generations before locking the store
            pHAInfo->SyncState |= 0x2;
            pHAInfo->SyncTime[2] = ism_common_currentTimeNanos();
            TRACE(5, "HASync: All the basic set of generation files have been sent to the Standby node. SyncState 0x%x\n", pHAInfo->SyncState);
            pthread_mutex_unlock(&pHAInfo->Mutex);
            continue;
         }

         if (!pHAInfo->fSyncLocked)
         {
            pthread_mutex_unlock(&pHAInfo->Mutex);
            if ((rc = ism_store_memHASyncLock()) == StoreRC_OK)
            {
               continue;
            }
            break;
         }

         pHAInfo->SyncState |= 0x4;
         TRACE(5, "HASync: All the required generation files have been sent to the Standby node. SyncState 0x%x\n", pHAInfo->SyncState);

         memset(&job, '\0', sizeof(job));
         job.JobType = StoreHAJob_SyncMemGen;
         ism_store_memHAAddJob(&job);
         pthread_mutex_unlock(&pHAInfo->Mutex);
         break;
      }

      // If the primary node creates a lot of new disk generations during the synchronization phase, the procedure
      // might take a long time. In such a case, we want to lock the store to avoid the primary to create new generations.
      elapsedTime = ism_common_currentTimeNanos() - pHAInfo->SyncTime[2];
      if (!pHAInfo->fSyncLocked && (pHAInfo->SyncState & 0x2) && elapsedTime > ismSTORE_HA_SYNC_MAX_TIME)
      {
         TRACE(5, "HASync: A lot of new generation files were created during the synchronization, thus we are going to lock the store. SyncMemSizeBytes %lu:%lu, ElapsedTime %0.2f",
               pHAInfo->SyncCurMemSizeBytes, pHAInfo->SyncMaxMemSizeBytes, (double)elapsedTime / 1e9);
         pthread_mutex_unlock(&pHAInfo->Mutex);
         if ((rc = ism_store_memHASyncLock()) == StoreRC_OK)
         {
            continue;
         }
         break;
      }

      if (pHAInfo->SyncCurMemSizeBytes + diskFileSize > pHAInfo->SyncMaxMemSizeBytes)
      {
         TRACE(7, "HASync: Waits for available memory to read a generation file (GenId %u, DiskFileSize %lu). SyncMemSizeBytes %lu:%lu\n",
               genId, diskFileSize, pHAInfo->SyncCurMemSizeBytes, pHAInfo->SyncMaxMemSizeBytes);
         if (pHAInfo->SyncCurMemSizeBytes == 0)
         {
            TRACE(1, "HASync: The generation file (GenId %u, DiskFileSize %lu) is too large. SyncMaxMemSizeBytes %lu\n",
                  genId, diskFileSize, pHAInfo->SyncMaxMemSizeBytes);
            rc = StoreRC_SystemError;
         }
         pthread_mutex_unlock(&pHAInfo->Mutex);
         break;
      }

      if ((pGenMap->pHASyncBuffer = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,114),diskFileSize)) == NULL)
      {
         TRACE(1, "HASync: Failed to allocate memory for a generation file (GenId %u, DiskFileSize %lu)\n",
               genId, diskFileSize);
         rc = StoreRC_SystemError;
         pthread_mutex_unlock(&pHAInfo->Mutex);
         break;
      }
      pGenMap->HASyncBufferLength = diskFileSize;
      pHAInfo->SyncCurMemSizeBytes += diskFileSize;

      // See whether we can read the generation data directly from the recovery memory
      memset(&buffParams, '\0', sizeof(buffParams));
      buffParams.pBuffer = pGenMap->pHASyncBuffer;
      buffParams.BufferLength = pGenMap->HASyncBufferLength;
      if ((ec = ism_store_memRecoveryGetGenerationData(genId, &buffParams)) == ISMRC_OK && buffParams.BufferLength <= pGenMap->HASyncBufferLength)
      {
         pGenMap->HASyncDataLength = buffParams.BufferLength;
         pGenMap->HASyncState = StoreHASyncGen_Available;
         TRACE(5, "HASync: The generation file (GenId %u, DiskFileSize %lu, HASyncDataLength %lu) has been read from the recovery memory\n",
             genId, diskFileSize, pGenMap->HASyncDataLength);

         memset(&job, '\0', sizeof(job));
         job.JobType = StoreHAJob_SyncDiskGen;
         job.GenId = genId;
         ism_store_memHAAddJob(&job);
      }
      else
      {
         // The generation data does not exist in the recovery memory. We need to read it from the disk
         memset(&diskTask, '\0', sizeof(diskTask));
         diskTask.GenId = genId;
         diskTask.Priority = ((pHAInfo->SyncState & 0x2) ? 0 : 1);
         diskTask.fCancelOnTerm = 1;
         diskTask.Callback = ism_store_memHASyncDiskReadComplete;
         diskTask.pContext = (void *)pGenMap->pHASyncBuffer;
         diskTask.BufferParams->pBuffer = pGenMap->pHASyncBuffer;
         diskTask.BufferParams->BufferLength = diskFileSize;

         TRACE(7, "HASync: Start reading a generation file (GenId %u, DiskFileSize %lu) from the primary disk (Priority %u)\n",
               genId, diskFileSize, diskTask.Priority);

         if ((rc = ism_storeDisk_readGeneration(&diskTask)) != StoreRC_OK)
         {
            TRACE(1, "HASync: Failed to read a generation file (GenId %u, DiskFileSize %lu) from the primary disk. error code %d\n",
                  genId, diskFileSize, rc);

            pthread_mutex_unlock(&pHAInfo->Mutex);
            break;
         }
         pGenMap->HASyncState = StoreHASyncGen_Reading;
      }
      pthread_mutex_unlock(&pHAInfo->Mutex);
   }

   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
   return rc;
}

static int ism_store_memHASyncSendDiskGen(ismStore_GenId_t genId)
{
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   ismStore_memGenMap_t *pGenMap;
   ismStore_memGenHeader_t *pGenHeader;
   ismStore_memHAMsgType_t msgType = StoreHAMsg_SyncDiskGen;
   char *pBuffer=NULL, *pPos;
   uint64_t dataLength, offset=0;
   uint32_t opcount, fragLength, bufferLength, requiredLength;
   int rc = StoreRC_OK;

   pGenMap = ismStore_memGlobal.pGensMap[genId];
   pGenHeader = (ismStore_memGenHeader_t *)pGenMap->pHASyncBuffer;

   // Sanity check
   if (pGenMap->HASyncDataLength == 0 || pGenMap->HASyncDataLength > pGenMap->HASyncBufferLength)
   {
      TRACE(1, "HASync: Failed to send a generation file (GenId %u, MemSizeBytes %lu, CompactedSizeBytes %lu) to the Standby node because the buffer length is too small (HASyncBufferLength %lu, HASyncDataLength %lu)\n",
            genId, pGenHeader->MemSizeBytes, pGenHeader->CompactSizeBytes, pGenMap->HASyncBufferLength, pGenMap->HASyncDataLength);
      return StoreRC_SystemError;
   }

   TRACE(7, "HASync: A generation file (GenId %u, DiskFileSize %lu, MemSizeBytes %lu, CompactedSizeBytes %lu, HASyncBufferLength %lu, HASyncDataLength %lu, HASyncState %u) is being sent to the Standby node\n",
         genId, pGenMap->DiskFileSize, pGenHeader->MemSizeBytes, pGenHeader->CompactSizeBytes,
         pGenMap->HASyncBufferLength, pGenMap->HASyncDataLength, pGenMap->HASyncState);

   dataLength = pGenMap->HASyncDataLength;
   while (dataLength > 0)
   {
      requiredLength = (dataLength < 1000000000 ? (uint32_t)dataLength : 1000000000);
      if ((rc = ism_store_memHAEnsureBufferAllocation(pHAInfo->pSyncChannel,
                                                      &pBuffer,
                                                      &bufferLength,
                                                      &pPos,
                                                      requiredLength,
                                                      msgType,
                                                      &opcount)) != StoreRC_OK)
      {
         TRACE(1, "HASync: Failed to send a message (MsgType %u, MsgSqn %lu, LastFrag %u) to the Standby node. error code %d\n",
               msgType, pHAInfo->pSyncChannel->MsgSqn, pHAInfo->pSyncChannel->FragSqn, rc);
         return rc;
      }

      // Add the header information in the first fragment only
      if (pHAInfo->pSyncChannel->FragSqn == 0)
      {
         ismSTORE_putShort(pPos, Operation_Null);
         ismSTORE_putInt(pPos, SHORT_SIZE + LONG_SIZE);
         ismSTORE_putLong(pPos, dataLength);
         ismSTORE_putShort(pPos, genId);
         opcount++;
      }

      fragLength = bufferLength - ((uint32_t)(pPos - pBuffer) + SHORT_SIZE + INT_SIZE + LONG_SIZE);
      if (dataLength < fragLength) {
         fragLength = dataLength;
      }

      ismSTORE_putShort(pPos, Operation_Null);
      ismSTORE_putInt(pPos, fragLength + LONG_SIZE);
      ismSTORE_putLong(pPos, offset);

      memcpy(pPos, pGenMap->pHASyncBuffer + offset, fragLength);
      pPos += fragLength;
      opcount++;

      dataLength -= fragLength;
      offset += fragLength;
      pHAInfo->SyncSentBytes += fragLength;
   }

   // Send the last fragment
   // requiredLength = 0 means that this is the last fragment.
   if ((rc = ism_store_memHAEnsureBufferAllocation(pHAInfo->pSyncChannel, &pBuffer, &bufferLength, &pPos, 0, msgType, &opcount)) != StoreRC_OK)
   {
      TRACE(1, "HASync: Failed to send a message (MsgType %u, MsgSqn %lu, LastFrag %u) to the Standby node. error code %d\n",
            msgType, pHAInfo->pSyncChannel->MsgSqn, pHAInfo->pSyncChannel->FragSqn, rc);
      return rc;
   }

   pthread_mutex_lock(&pHAInfo->Mutex);
   pHAInfo->SyncSentGensCount++;
   pGenMap->HASyncState = StoreHASyncGen_Sent;
   pHAInfo->SyncCurMemSizeBytes -= pGenMap->HASyncBufferLength;
   pGenMap->HASyncBufferLength = 0;
   pGenMap->HASyncDataLength = 0;
   ismSTORE_FREE(pGenMap->pHASyncBuffer);
   pthread_mutex_unlock(&pHAInfo->Mutex);

   TRACE(7, "HASync: A generation file (GenId %u, DiskFileSize %lu) has been sent to the Standby node. SyncSentGensCount %u, SyncExpGensCount %u\n",
         genId, pGenMap->DiskFileSize, pHAInfo->SyncSentGensCount, pHAInfo->SyncExpGensCount);

   rc = ism_store_memHASyncReadDiskGen();

   return rc;
}

static int ism_store_memHASyncLock(void)
{
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memGenHeader_t *pGenHeader;
   uint8_t genIndex, activeGenIndex;
   int rc,i;

   TRACE(9, "Entry: %s\n", __FUNCTION__);
   for ( i=0 ; i<3 ; i++ )
   {
      if (ism_store_memLockStore(3300,LOCKSTORE_CALLER_SYNC))
         break ; 
      else
      {
         if ( i < 2 )
         {
            TRACE(1,"ism_store_memLockStore failed after %u secs, will ism_store_memUnlockStore() and retry...\n",i*3300/1000);
            ism_common_stack_trace(0);
            ism_store_memUnlockStore(LOCKSTORE_CALLER_SYNC);
            ism_common_sleep(33000);  // 33 ms
         }
         else
         {
            TRACE(1,"ism_store_memLockStore failed after 10 sec, will stop the sync process.\n");
            ism_common_stack_trace(0);
            ism_store_memUnlockStore(LOCKSTORE_CALLER_SYNC);
            TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, StoreRC_HA_ConnectionBroke);
            return StoreRC_HA_ConnectionBroke;
         }
      }
   }
   pHAInfo->SyncTime[3] = ism_common_currentTimeNanos();

   // Wait until there are no generations during DiskWrite
   // We have to skip on this loop during startup, because the generation's state is not changed
   if (ismStore_memGlobal.State != ismSTORE_STATE_INIT)
   {
      pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;
      activeGenIndex = pMgmtHeader->ActiveGenIndex;

      pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);
      for (i=0; i < ismStore_memGlobal.InMemGensCount; i++)
      {
         genIndex = (activeGenIndex + i) % ismStore_memGlobal.InMemGensCount;
         pGenHeader = (ismStore_memGenHeader_t *)ismStore_memGlobal.InMemGens[genIndex].pBaseAddress;

         while (pHAInfo->State == ismSTORE_HA_STATE_PRIMARY &&   /* BEAM suppression: infinite loop */
            ((pGenHeader->State != ismSTORE_GEN_STATE_FREE &&
                pGenHeader->State != ismSTORE_GEN_STATE_ACTIVE) ||
                pGenHeader->GenId == ismSTORE_RSRV_GEN_ID))
         {
            pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
            ism_common_sleep(100000);
            pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);
         }
      }
      pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
   }

   pthread_mutex_lock(&pHAInfo->Mutex);
   pHAInfo->fSyncLocked = 1;
   rc = (pHAInfo->State == ismSTORE_HA_STATE_PRIMARY ? StoreRC_OK : StoreRC_HA_ConnectionBroke);
   pthread_mutex_unlock(&pHAInfo->Mutex);

   if (rc == StoreRC_OK)
   {
      TRACE(5, "HASync: The store is locked for new node synchronization\n");
   }
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
   return rc;
}

static int ism_store_memHASyncSendMemGen(void)
{
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   ismStore_memHAJob_t job;
   ismStore_memHAAck_t ack;
   ismStore_memHAMsgType_t msgType = StoreHAMsg_SyncMemGen;
   ismStore_memGeneration_t *pGen;
   ismStore_memGenHeader_t *pGenHeader;
   ismStore_memGenMap_t *pGenMap, genMap;
   ismStore_DiskBufferParams_t buffParams;
   double compactRatio;
   char *pData, *pBuffer=NULL, *pPos;
   uint64_t dataLength, offset, *pBitMapsArray[ismSTORE_GRANULE_POOLS_COUNT];
   uint32_t opcount, fragLength, bufferLength, requiredLength;
   uint8_t poolId;
   int rc = StoreRC_OK, ec, i;

   pGenMap = ismStore_memGlobal.pGensMap[ismSTORE_MGMT_GEN_ID];
   TRACE(9, "Entry: %s. TotalMemSizeBytes %lu, InMemGensCount %d, HASyncState %u\n",
         __FUNCTION__, ismStore_memGlobal.TotalMemSizeBytes, ismStore_memGlobal.InMemGensCount, pGenMap->HASyncState);

   if (pGenMap->HASyncState == StoreHASyncGen_Requested)
   {
      for (i=-1; i < ismStore_memGlobal.InMemGensCount; i++)
      {
         pGen = (i == -1 ? &ismStore_memGlobal.MgmtGen : &ismStore_memGlobal.InMemGens[i]);
         pGenHeader = (ismStore_memGenHeader_t *)pGen->pBaseAddress;
         pGenMap = ismStore_memGlobal.pGensMap[pGenHeader->GenId];
         pData = (char *)pGenHeader;
         dataLength = pGenHeader->MemSizeBytes;

         memset(&buffParams, '\0', sizeof(buffParams));
         memset(&genMap, '\0', sizeof(genMap));
         memset(pBitMapsArray, '\0', sizeof(pBitMapsArray));
         compactRatio = 0;

         // Optimization: If the memory generation is free, we don't need to send the whole generation but only the header
         if (pGenHeader->State == ismSTORE_GEN_STATE_FREE)
         {
            dataLength = sizeof(ismStore_memGenHeader_t);
         }
         else  // Compact the memory generation data before sending it to the Standby node
         {
            // There is no need to build a BitMap for the management generation
            if ((pGenHeader->GenId == ismSTORE_MGMT_GEN_ID) || (ec = ism_store_memCreateGranulesMap(pGenHeader, &genMap, 0)) == ISMRC_OK)
            {
               if (pGenHeader->GenId != ismSTORE_MGMT_GEN_ID)
               {
                  memset(pBitMapsArray, '\0', sizeof(pBitMapsArray));
                  for (poolId=0; poolId < genMap.GranulesMapCount; poolId++) { pBitMapsArray[poolId] = genMap.GranulesMap[poolId].pBitMap[ismSTORE_BITMAP_LIVE_IDX]; }
                  buffParams.pBitMaps = pBitMapsArray;
               }
               if ((ec = ism_storeDisk_compactGenerationData((void *)pGenHeader, &buffParams)) == StoreRC_OK)
               {
                  pData = buffParams.pBuffer;
                  dataLength = buffParams.BufferLength;
                  compactRatio = 1.0 - (double)buffParams.BufferLength / pGenHeader->MemSizeBytes;
                  TRACE(5, "HASync: Store memory generation Id %u (Index %u) has been compacted. MemSizeBytes %lu, CompactedSizeBytes %lu, CompactRatio %f, StdDevBytes %lu\n",
                        pGenHeader->GenId, i, pGenHeader->MemSizeBytes, dataLength, compactRatio, buffParams.StdDev);
               }
               else
               {
                  TRACE(4, "HASync: Failed to compact the memory generation Id %u (Index %u), because the ism_storeDisk_compactGenerationData failed. MemSizeBytes %lu, error code %d\n",
                        pGenHeader->GenId, i, dataLength, ec);
               }
            }
            else
            {
               TRACE(4, "HASync: Failed to compact the memory generation Id %u (Index %u), because ism_store_memCreateGranulesMap failed. error code %d\n",
                     pGenHeader->GenId, i, ec);
            }
            for (poolId=0; poolId < ismSTORE_GRANULE_POOLS_COUNT; poolId++) { ismSTORE_FREE(genMap.GranulesMap[poolId].pBitMap[ismSTORE_BITMAP_LIVE_IDX]); }
            buffParams.pBitMaps = NULL;
         }

         offset = 0;
         TRACE(7, "HASync: The memory generation (GenId %u, Index %d, MemSizeBytes %lu, State %d) is being sent to the Standby node. dataLength %lu, CompactRatio %f, HASyncState %d\n",
               pGenHeader->GenId, i, pGenHeader->MemSizeBytes, pGenHeader->State, dataLength, compactRatio, pGenMap->HASyncState);
         while (dataLength > 0)
         {
            requiredLength = (dataLength < 1000000000 ? (uint32_t)dataLength : 1000000000);
            if ((rc = ism_store_memHAEnsureBufferAllocation(pHAInfo->pSyncChannel,
                                                            &pBuffer,
                                                            &bufferLength,
                                                            &pPos,
                                                            requiredLength,
                                                            msgType,
                                                            &opcount)) != StoreRC_OK)
            {
               TRACE(1, "HASync: Failed to send a message (MsgType %u, MsgSqn %lu, LastFrag %u) to the Standby node. error code %d\n",
                     msgType, pHAInfo->pSyncChannel->MsgSqn, pHAInfo->pSyncChannel->FragSqn, rc);
               goto exit;
            }

            // Add the header information in the first fragment only
            if (pHAInfo->pSyncChannel->FragSqn == 0)
            {
               ismSTORE_putShort(pPos, Operation_Null);
               ismSTORE_putInt(pPos, SHORT_SIZE + LONG_SIZE + 1);
               ismSTORE_putLong(pPos, dataLength);
               ismSTORE_putShort(pPos, pGenHeader->GenId);
               ismSTORE_putChar(pPos, (uint8_t)i);
               opcount++;
            }

            fragLength = bufferLength - ((uint32_t)(pPos - pBuffer) + SHORT_SIZE + INT_SIZE + LONG_SIZE);
            if (dataLength < fragLength) {
               fragLength = dataLength;
            }

            ismSTORE_putShort(pPos, Operation_Null);
            ismSTORE_putInt(pPos, fragLength + LONG_SIZE);
            ismSTORE_putLong(pPos, offset);

            memcpy(pPos, pData + offset, fragLength);
            pPos += fragLength;
            opcount++;

            dataLength -= fragLength;
            offset += fragLength;
            pHAInfo->SyncSentBytes += fragLength;
         }

         // Send the last fragment
         // requiredLength = 0 means that this is the last fragment.
         memset(&ack, '\0', sizeof(ack));
         ack.AckSqn = pHAInfo->pSyncChannel->MsgSqn;
         if ((rc = ism_store_memHAEnsureBufferAllocation(pHAInfo->pSyncChannel, &pBuffer, &bufferLength, &pPos, 0, msgType, &opcount)) != StoreRC_OK)
         {
            TRACE(1, "HASync: Failed to send a message (MsgType %u, MsgSqn %lu, LastFrag %u) to the Standby node. error code %d\n",
                  msgType, pHAInfo->pSyncChannel->MsgSqn, pHAInfo->pSyncChannel->FragSqn, rc);
            goto exit;
         }
         ism_common_free_raw(ism_memory_store_misc,buffParams.pBuffer);
         TRACE(7, "HASync: The memory generation (GenId %u) has been sent to the Standby node\n", pGenHeader->GenId);

         // Wait for an ACK from the Standby node
         if ((rc = ism_store_memHAReceiveAck(pHAInfo->pSyncChannel, &ack, 0)) != StoreRC_OK)
         {
            TRACE(1, "HASync: Failed to receive an ACK from the Standby node. MsgType %u, MsgSqn %lu, LastFrag %u, error code %d\n",
                  msgType, pHAInfo->pSyncChannel->MsgSqn, pHAInfo->pSyncChannel->FragSqn, rc);
            goto exit;
         }

         if (ack.ReturnCode != StoreRC_OK)
         {
            TRACE(1, "HASync: Failed to process a message on the Standby node. MsgType %u, MsgSqn %lu, LastFrag %u, error code %d\n",
                  msgType, pHAInfo->pSyncChannel->MsgSqn, pHAInfo->pSyncChannel->FragSqn, ack.ReturnCode);
            rc = ack.ReturnCode;
            goto exit;
         }

         TRACE(7, "HASync: The memory generation (GenId %u) has been received by the Standby node\n", pGenHeader->GenId);
         pGenMap->HASyncState = StoreHASyncGen_Sent;
      }
   }
   else if (pGenMap->HASyncState == StoreHASyncGen_Proposed)
   {
      TRACE(7, "HASync: There is no need to send the memory generations to the Standby node\n");
   }
   else
   {
      TRACE(1, "HASync: The state (HASyncState %d) of the management generation is not valid\n", pGenMap->HASyncState);
      rc = StoreRC_SystemError;
      goto exit;
   }

   pthread_mutex_lock(&pHAInfo->Mutex);
   pHAInfo->SyncSentGensCount++;
   pHAInfo->SyncState |= 0x8;
   TRACE(5, "HASync: All the memory generations have been sent to the Standby node. SyncState 0x%x, SyncSentGensCount %u, SyncExpGensCount %u, GensCount %u\n",
         pHAInfo->SyncState, pHAInfo->SyncSentGensCount, pHAInfo->SyncExpGensCount, (ismStore_memGlobal.GensMapCount - 1));

   memset(&job, '\0', sizeof(job));
   job.JobType = StoreHAJob_SyncComplete;
   ism_store_memHAAddJob(&job);
   pthread_mutex_unlock(&pHAInfo->Mutex);

exit:
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
   return rc;
}

static void ism_store_memHASyncDiskReadComplete(ismStore_GenId_t genId, int32_t retcode, ismStore_DiskGenInfo_t *pDiskGenInfo, void *pContext)
{
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   ismStore_memGenMap_t *pGenMap;
   ismStore_memGenHeader_t *pGenHeader;
   ismStore_memHAJob_t job;

   pthread_mutex_lock(&pHAInfo->Mutex);
   pGenMap = ismStore_memGlobal.pGensMap[genId];
   if (pGenMap->HASyncState == StoreHASyncGen_Reading)
   {
      memset(&job, '\0', sizeof(job));
      job.GenId = genId;
      if (retcode == StoreRC_OK)
      {
         // Sanity check
         pGenHeader = (ismStore_memGenHeader_t *)pGenMap->pHASyncBuffer;
         if (pDiskGenInfo->DataLength == 0 || pDiskGenInfo->DataLength > pGenMap->HASyncBufferLength)
         {
            TRACE(1, "HASync: Failed to read a generation file (GenId %u, DiskFileSize %lu, MemSizeBytes %lu, CompactedSizeBytes %lu) from the primary disk because the buffer length is too small (HASyncBufferLength %lu, DataLength %lu)\n",
                  genId, pDiskGenInfo->DataLength, pGenHeader->MemSizeBytes, pGenHeader->CompactSizeBytes, pGenMap->HASyncBufferLength, pDiskGenInfo->DataLength);
            pGenMap->HASyncState = StoreHASyncGen_Error;
            job.JobType = StoreHAJob_SyncAbort;
         }
         else
         {
            TRACE(5, "HASync: The generation file (GenId %u, DiskFileSize %lu, HASyncBufferLength %lu, HASyncDataLength %lu, HASyncState %u) has been read from the primary disk\n",
                  genId, pDiskGenInfo->DataLength, pGenMap->HASyncBufferLength, pDiskGenInfo->DataLength, pGenMap->HASyncState);

            pGenMap->HASyncDataLength = pDiskGenInfo->DataLength;
            pGenMap->HASyncState = StoreHASyncGen_Available;
            job.JobType = StoreHAJob_SyncDiskGen;
         }
      }
      else
      {
         TRACE(1, "HASync: Failed to read a generation file (GenId %u, DiskFileSize %lu, HASyncBufferLength %lu, HASyncState %u) from the primary disk. error code %d\n",
               genId, pDiskGenInfo->DataLength, pGenMap->HASyncBufferLength, pGenMap->HASyncState, retcode);
         pGenMap->HASyncState = StoreHASyncGen_Error;
         job.JobType = StoreHAJob_SyncAbort;
      }
      ism_store_memHAAddJob(&job);
   }
   else
   {
      TRACE(1, "HASync: The synchronization state (%u) of the generation file (GenId %u, DiskFileState %lu, HASyncBufferLength %lu) is not valid. error code %d\n",
            pGenMap->HASyncState, genId, pGenMap->DiskFileSize, pGenMap->HASyncBufferLength, retcode);
      ismSTORE_FREE(pContext);
   }
   pthread_mutex_unlock(&pHAInfo->Mutex);
}

static void ism_store_memHASyncDiskWriteComplete(ismStore_GenId_t genId, int32_t retcode, ismStore_DiskGenInfo_t *pDiskGenInfo, void *pContext)
{
   ismStore_memHAFragment_t *pFrag = (ismStore_memHAFragment_t *)pContext;
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;

   pthread_mutex_lock(&pHAInfo->Mutex);

   if (retcode == StoreRC_OK || retcode == StoreRC_Disk_TaskInterrupted || retcode == StoreRC_Disk_TaskCancelled)
   {
      TRACE(5, "HASync: A generation file (GenId %u, FileSize %lu) has been written to the Standby disk. SyncMemSizeBytes %lu:%lu, return code %d\n",
            genId, pFrag->DataLength, pHAInfo->SyncCurMemSizeBytes, pHAInfo->SyncMaxMemSizeBytes, retcode);
   }
   else
   {
      TRACE(1, "HASync: Failed to write a generation file (GenId %u, FileSize %lu) to the Standby disk. State %d, SyncMemSizeBytes %lu:%lu, error code %d\n",
            genId, pFrag->DataLength, pHAInfo->State, pHAInfo->SyncCurMemSizeBytes, pHAInfo->SyncMaxMemSizeBytes, retcode);

      // See whether a late diskComplete is received (the node is already acting as the standby node)
      if (pHAInfo->State == ismSTORE_HA_STATE_STANDBY)
      {
         TRACE(1, "HASync: The node is acting as a standby node but the new node synchronization has failed\n");
         ism_storeHA_sbError();
      }
      pHAInfo->SyncRC = ISMRC_StoreDiskError;
   }

   pHAInfo->SyncCurMemSizeBytes -= pFrag->DataLength;
   pthread_cond_signal(&pHAInfo->SyncCond);
   pthread_mutex_unlock(&pHAInfo->Mutex);

   ismSTORE_FREE(pFrag);
}

/*********************************************************************/
/* Store Disk Operations for High-Availability                       */
/*********************************************************************/
static void ism_store_memHADiskWriteComplete(ismStore_GenId_t genId, int32_t retcode, ismStore_DiskGenInfo_t *pDiskGenInfo, void *pContext)
{
   ismStore_memHADiskWriteCtxt_t *pDiskWriteCtxt = (ismStore_memHADiskWriteCtxt_t *)pContext;
   ismStore_memGeneration_t *pGen = (ismStore_memGeneration_t *)pDiskWriteCtxt->pGen;
   ismStore_memGenHeader_t *pGenHeader;
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   int ec;

   if (retcode == StoreRC_OK || retcode == StoreRC_Disk_TaskCancelled)
   {
      TRACE(5, "HADisk: Store generation Id %u has been written to the Standby disk. ViewId %u, CurrentViewId %u, CurrentRole %u, State %u, return code %d\n",
            genId, pDiskWriteCtxt->ViewId, pHAInfo->View.ViewId, pHAInfo->View.NewRole, pHAInfo->State, retcode);

      pDiskWriteCtxt->Ack.ReturnCode = StoreRC_OK;
   }
   else
   {
      TRACE(1, "HADisk: Failed to write store generation Id %u to the Standby disk. error code %d\n", genId, retcode);
      pDiskWriteCtxt->Ack.ReturnCode = StoreRC_SystemError;
   }

   pthread_mutex_lock(&pHAInfo->Mutex);
   if (pHAInfo->State == ismSTORE_HA_STATE_PRIMARY)
   {
      pthread_mutex_unlock(&pHAInfo->Mutex);
      TRACE(5, "HADisk: The HA view (ViewId %u) was changed. The node is now acting as primary. MsgSqn %lu, GenId %u, CurrentViewId %u, CurrentRole %u\n",
            pDiskWriteCtxt->ViewId, pDiskWriteCtxt->Ack.AckSqn, genId, pHAInfo->View.ViewId, pHAInfo->View.NewRole);
      ism_store_memDiskWriteComplete(genId, retcode, pDiskGenInfo, pContext);
      return;
   }
   else
   {
      if (retcode == StoreRC_OK || retcode == StoreRC_Disk_TaskCancelled)
      {
         pGenHeader = (ismStore_memGenHeader_t *)pGen->pBaseAddress;
         pGenHeader->State = ismSTORE_GEN_STATE_WRITE_COMPLETED;
         ADR_WRITE_BACK(&pGenHeader->State, sizeof(pGenHeader->State));
         TRACE(5, "HADisk: The state of generation Id %u has been changed to %u\n", genId, ismSTORE_GEN_STATE_WRITE_COMPLETED);
      }

      if (pHAInfo->State == ismSTORE_HA_STATE_STANDBY &&
          pDiskWriteCtxt->ViewId == pHAInfo->View.ViewId &&
          pDiskWriteCtxt->pHAChannel == pHAInfo->pIntChannel)
      {
         pthread_mutex_unlock(&pHAInfo->Mutex);
         if ((ec = ism_store_memHASendAck(pDiskWriteCtxt->pHAChannel, &pDiskWriteCtxt->Ack)) != StoreRC_OK)
         {
            TRACE(1, "HADisk: Failed to send back ACK for HA message (ChannelId %d, MsgType %u, MsgSqn %lu). error code %d\n",
                pDiskWriteCtxt->pHAChannel->ChannelId, pDiskWriteCtxt->Ack.SrcMsgType, pDiskWriteCtxt->Ack.AckSqn, ec);
         }
      }
      else
      {
        pthread_mutex_unlock(&pHAInfo->Mutex);
          TRACE(5, "HADisk: The HA view (ViewId %u) was changed. No need to send an HA ACK. MsgSqn %lu, GenId %u, CurrentViewId %u, CurrentRole %u\n",
                pDiskWriteCtxt->ViewId, pDiskWriteCtxt->Ack.AckSqn, genId, pHAInfo->View.ViewId, pHAInfo->View.NewRole);
      }
      if (pDiskWriteCtxt->Ack.ReturnCode == StoreRC_SystemError)
      {
         ism_storeHA_sbError();
      }
   }

   ismSTORE_FREE(pDiskWriteCtxt->pCompData);
   ismSTORE_FREE(pDiskWriteCtxt);
}

static void ism_store_memHADiskDeleteComplete(ismStore_GenId_t genId, int32_t retcode, ismStore_DiskGenInfo_t *pDiskGenInfo, void *pContext)
{
   if (retcode == StoreRC_OK)
   {
      TRACE(5, "HADisk: Store generation Id %u has been deleted from the Standby disk\n", genId);
   }
   else
   {
      TRACE(1, "HADisk: Failed to delete store generation Id %u from the Standby disk. error code %d\n", genId, retcode);
   }
}

static void ism_store_memHADiskDeleteDeadComplete(ismStore_GenId_t genId, int32_t retcode, ismStore_DiskGenInfo_t *pDiskGenInfo, void *pContext)
{
   if (retcode == StoreRC_OK)
   {
      TRACE(5, "HADisk: The dead generation files has been deleted from the Standby disk\n");
   }
   else
   {
      TRACE(1, "HADisk: Failed to delete dead generation files from the Standby disk. error code %d\n", retcode);
   }
}

/*********************************************************************/
/* Store High-Availability Operations for Admin layer                */
/*********************************************************************/
XAPI int ism_ha_store_get_view(ismHA_View_t *pView)
{
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   int rc = ISMRC_OK;

   TRACE(9, "Entry: %s\n", __FUNCTION__);

   if (!pView)
   {
      rc = ISMRC_NullArgument;
      ism_common_setError(rc);
      return rc;
   }
   memset(pView, '\0', sizeof(*pView));

   pthread_mutex_lock(&ismStore_HAAdminMutex);
   if (ismStore_global.fHAEnabled && pHAInfo->State != ismSTORE_HA_STATE_CLOSED && pHAInfo->State != ismSTORE_HA_STATE_TERMINATING)
   {
      pthread_mutex_lock(&pHAInfo->Mutex);
      memcpy(pView, &pHAInfo->AdminView, sizeof(*pView));
      pthread_mutex_unlock(&pHAInfo->Mutex);
   }
   else
   {
      pView->OldRole = pView->NewRole = ISM_HA_ROLE_DISABLED;
   }
   pthread_mutex_unlock(&ismStore_HAAdminMutex);

   TRACE(9, "Exit: %s. NewRole %u, OldRole %u, ActiveNodesCount %u, SyncNodesCount %u, ReasonCode %d, rc %d\n",
         __FUNCTION__, pView->NewRole, pView->OldRole, pView->ActiveNodesCount, pView->SyncNodesCount, pView->ReasonCode, rc);

   return rc;
}

XAPI int ism_ha_store_transfer_state_completed(int adminRC)
{
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   ismStore_memHAJob_t job;
   double adminTime;
   int rc = ISMRC_OK;

   TRACE(5, "Entry: %s. State %u, SyncState 0x%x, adminRC %d\n", __FUNCTION__, pHAInfo->State, pHAInfo->SyncState, adminRC);
   pthread_mutex_lock(&ismStore_HAAdminMutex);

   if (pHAInfo->State != ismSTORE_HA_STATE_PRIMARY || pHAInfo->SyncState == 0)
   {
      rc = ISMRC_StoreHAError;
      ism_common_setError(rc);
      pthread_mutex_unlock(&ismStore_HAAdminMutex);
      return rc;
   }

   pthread_mutex_lock(&pHAInfo->Mutex);
   pHAInfo->SyncTime[1] = ism_common_currentTimeNanos();
   if (getenv("FakeAdminOK") || adminRC == ISMRC_OK)
   {
      pHAInfo->SyncState |= 0x40;
   }
   else
   {
      TRACE(1, "HAAdmin: The new node synchronization procedure is being aborted by the admin. SyncState 0x%x, error code %d\n", pHAInfo->SyncState, adminRC);
      pHAInfo->SyncState |= 0x80;
      pHAInfo->SyncRC = adminRC;
      memset(&job, '\0', sizeof(job));
      job.JobType = StoreHAJob_SyncAbort;
      ism_store_memHAAddJob(&job);
   }
   pthread_cond_signal(&pHAInfo->SyncCond);
   pthread_mutex_unlock(&pHAInfo->Mutex);
   pthread_mutex_unlock(&ismStore_HAAdminMutex);

   adminTime = (double)(pHAInfo->SyncTime[1] - pHAInfo->SyncTime[0]) / 1e9;
   TRACE(5, "Exit: %s. State %u, SyncState 0x%x, AdminTime %.6f, rc %d\n", __FUNCTION__, pHAInfo->State, pHAInfo->SyncState, adminTime, rc);

   return rc;
}

XAPI int ism_ha_store_send_admin_msg(ismHA_AdminMessage_t *pAdminMsg)
{
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   ismStore_memHAAck_t ack;
   char *pBuffer=NULL, *pPos;
   uint8_t fBusyWarn = 0;
   uint32_t dataLength, bufferLength, fragLength, opcount;
   uint64_t offset=0;
   int rc = ISMRC_OK, ec;

   if (!pAdminMsg || !pAdminMsg->pData)
   {
      TRACE(1, "HAAdmin: Failed to send an HA admin message, because the arguments are not valid\n");
      rc = ISMRC_NullArgument;
      ism_common_setError(rc);
      return rc;
   }

   if (pAdminMsg->DataLength == 0)
   {
      TRACE(1, "HAAdmin: Failed to send an HA admin message, because the arguments are not valid\n");
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%u", "DataLength", pAdminMsg->DataLength);
      return rc;
   }

   TRACE(9, "Entry: %s. DataLength %u, ResBufferLnegth %u\n", __FUNCTION__, pAdminMsg->DataLength, pAdminMsg->ResBufferLength);

   pthread_mutex_lock(&ismStore_HAAdminMutex);
   while (1)
   {
      if (pHAInfo->State != ismSTORE_HA_STATE_PRIMARY || !pHAInfo->pAdminChannel)
      {
         TRACE(1, "HAAdmin: Failed to send an HA admin message (DataLength %u), because the node state is not valid. State %d, SyncNodesCount %u, pAdminChannel %p\n",
               pAdminMsg->DataLength, pHAInfo->State, pHAInfo->SyncNodesCount, pHAInfo->pAdminChannel);
         rc = ISMRC_StoreHAError;
         pthread_mutex_unlock(&ismStore_HAAdminMutex);
         goto exit;
      }
      if (!pHAInfo->fAdminChannelBusy) { break; }

      if (!fBusyWarn)
      {
         TRACE(4, "HAAdmin: Wait until a previous admin task completes\n");
         fBusyWarn = 1;
      }

      pthread_mutex_unlock(&ismStore_HAAdminMutex);
      ism_common_sleep(100000);
      pthread_mutex_lock(&ismStore_HAAdminMutex);
   }

   if (fBusyWarn)
   {
      TRACE(4, "HAAdmin: The previous admin task has been completed\n");
   }

   pHAInfo->fAdminChannelBusy = 1;
   pthread_mutex_unlock(&ismStore_HAAdminMutex);

   for (dataLength = pAdminMsg->DataLength; dataLength > 0; )
   {
      if ((ec = ism_store_memHAEnsureBufferAllocation(pHAInfo->pAdminChannel,
                                                      &pBuffer,
                                                      &bufferLength,
                                                      &pPos,
                                                      dataLength,
                                                      StoreHAMsg_Admin,
                                                      &opcount)) != StoreRC_OK)
      {
         TRACE(1, "HAAdmin: Failed to send an HA admin message. error code %d\n", ec);
         rc = ISMRC_StoreHAError;
         goto exit;
      }

      // Add the header information in the first fragment only
      if (pHAInfo->pAdminChannel->FragSqn == 0)
      {
         ismSTORE_putShort(pPos, Operation_Null);
         ismSTORE_putInt(pPos, LONG_SIZE);
         ismSTORE_putLong(pPos, dataLength);
         opcount++;
      }

      fragLength = bufferLength - ((uint32_t)(pPos - pBuffer) + SHORT_SIZE + INT_SIZE + LONG_SIZE);
      if (dataLength < fragLength) {
         fragLength = dataLength;
      }

      ismSTORE_putShort(pPos, Operation_Null);
      ismSTORE_putInt(pPos, fragLength + LONG_SIZE);
      ismSTORE_putLong(pPos, offset);

      memcpy(pPos, pAdminMsg->pData + offset, fragLength);
      pPos += fragLength;
      opcount++;

      dataLength -= fragLength;
      offset += fragLength;
   }

   // Send the last fragment
   // requiredLength = 0 means that this is the last fragment.
   memset(&ack, '\0', sizeof(ack));
   ack.AckSqn = pHAInfo->pAdminChannel->MsgSqn;
   if ((ec = ism_store_memHAEnsureBufferAllocation(pHAInfo->pAdminChannel, &pBuffer, &bufferLength, &pPos, 0, StoreHAMsg_Admin, &opcount)) != StoreRC_OK)
   {
      TRACE(1, "HAAdmin: Failed to send an HA admin message. error code %d\n", ec);
      rc = ISMRC_StoreHAError;
      goto exit;
   }

   TRACE(9, "HAAdmin: An admin message has been sent. ChannelId %d, MsgSqn %lu, LastFrag %u\n",
         pHAInfo->pAdminChannel->ChannelId, pHAInfo->pAdminChannel->MsgSqn - 1, pHAInfo->pAdminChannel->FragSqn);

   // Wait for an ACK from the Standby node
   ack.pData = pAdminMsg->pResBuffer;
   ack.DataLength = pAdminMsg->ResBufferLength;
   ec = ism_store_memHAReceiveAck(pHAInfo->pAdminChannel, &ack, 0);
   if (ec != StoreRC_OK)
   {
      TRACE(1, "HAAdmin: Failed to receive ACK for admin message. MsgSqn %lu, LastFrag %u, error code %d\n",
            pHAInfo->pAdminChannel->MsgSqn - 1, pHAInfo->pAdminChannel->FragSqn, ec);
      rc = ISMRC_StoreHAError;
      goto exit;
   }

   if (ack.ReturnCode != StoreRC_OK)
   {
      TRACE(1, "HAAdmin: Failed to process an admin message on the Standby node. MsgSqn %lu, LastFrag %u, error code %d\n",
            pHAInfo->pAdminChannel->MsgSqn - 1, pHAInfo->pAdminChannel->FragSqn, ack.ReturnCode);
      rc = ISMRC_StoreHAError;
      goto exit;
   }

   pAdminMsg->ResLength = ack.DataLength;

exit:
   if (rc != ISMRC_OK)
   {
      ism_common_setError(rc);
   }

   pthread_mutex_lock(&ismStore_HAAdminMutex);
   pHAInfo->fAdminChannelBusy = 0;
   pthread_mutex_unlock(&ismStore_HAAdminMutex);

   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

XAPI int ism_ha_store_transfer_file(char *pPath, char *pFilename)
{
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   ismStore_DiskTaskParams_t diskTask;
   ismStore_memHAAck_t ack;
   ismStore_memHAMsgType_t msgType = StoreHAMsg_AdminFile;
   char *pBuffer=NULL, *pPos;
   uint8_t fBusyWarn = 0;
   uint32_t bufferLength, fragLength, requiredLength, plen, flen, opcount;
   uint64_t dataLength, offset=0;
   int rc = ISMRC_OK, ec;

   if (!pPath || !pFilename)
   {
      TRACE(1, "HAAdmin: Failed to transfer an admin file, because the arguments are not valid\n");
      rc = ISMRC_NullArgument;
      return rc;
   }

   TRACE(9, "Entry: %s. Path %s, Filename %s\n", __FUNCTION__, pPath, pFilename);

   memset(&diskTask, '\0', sizeof(diskTask));
   diskTask.Priority = 0;
   diskTask.Callback = ism_store_memHAAdminDiskReadComplete;
   diskTask.Path = pPath;
   diskTask.File = pFilename;

   pthread_mutex_lock(&ismStore_HAAdminMutex);
   while (1)
   {
      if (pHAInfo->State != ismSTORE_HA_STATE_PRIMARY || !pHAInfo->pAdminChannel)
      {
         TRACE(1, "HAAdmin: Failed to transfer an admin file (Filename %s), because the node state is not valid. State %d, SyncNodesCount %u, pAdminChannel %p\n",
               pFilename, pHAInfo->State, pHAInfo->SyncNodesCount, pHAInfo->pAdminChannel);
         rc = ISMRC_StoreHAError;
         pthread_mutex_unlock(&ismStore_HAAdminMutex);
         goto exit;
      }
      if (!pHAInfo->fAdminChannelBusy) { break; }

      if (!fBusyWarn)
      {
         TRACE(4, "HAAdmin: Wait until a previous admin task completes\n");
         fBusyWarn = 1;
      }

      pthread_mutex_unlock(&ismStore_HAAdminMutex);
      ism_common_sleep(100000);
      pthread_mutex_lock(&ismStore_HAAdminMutex);
   }

   if (fBusyWarn)
   {
      TRACE(4, "HAAdmin: The previous admin task has been completed\n");
   }

   pHAInfo->fAdminChannelBusy = 1;
   pHAInfo->AdminFileState = 1;
   pthread_mutex_unlock(&ismStore_HAAdminMutex);

   if ((ec = ism_storeDisk_getFileSize(pPath, pFilename, &diskTask.BufferParams->BufferLength)) != StoreRC_OK)
   {
      TRACE(1, "HAAdmin: Failed to transfer an admin file, because of a disk failure. Filename %s, error code %d\n",
            pFilename, ec);
      rc = ISMRC_StoreDiskError;
      goto exit;
   }

   // Create a job for the HA Admin thread, including the data to be sent
   if ((diskTask.BufferParams->pBuffer = (char *)ism_common_malloc_noeyecatcher(diskTask.BufferParams->BufferLength)) == NULL)
   {
      TRACE(1, "HAAdmin: Failed to allocate memory for an admin file. Filename %s, FileSize %lu\n",
           pFilename, diskTask.BufferParams->BufferLength);
      rc = ISMRC_AllocateError;
      goto exit;
   }

   TRACE(9, "HAAdmin: Start reading an admin file from the primary disk. Filename %s, FileSize %lu\n",
         pFilename, diskTask.BufferParams->BufferLength);

   if ((ec = ism_storeDisk_readFile(&diskTask)) != StoreRC_OK)
   {
      TRACE(1, "HAAdmin: Failed to transfer an admin file because of a disk read failure. Filename %s, FileSize %lu, error code %d\n",
            pFilename, diskTask.BufferParams->BufferLength, ec);
      rc = ISMRC_StoreDiskError;
      goto exit;
   }

   // Wait for disk read completion
   pthread_mutex_lock(&ismStore_HAAdminMutex);
   while (pHAInfo->AdminFileState == 1 && pHAInfo->State == ismSTORE_HA_STATE_PRIMARY && pHAInfo->pAdminChannel )   /* BEAM suppression: infinite loop */
   {
      pthread_mutex_unlock(&ismStore_HAAdminMutex);
      ism_common_sleep(1000);
      pthread_mutex_lock(&ismStore_HAAdminMutex);
   }

   if (pHAInfo->State != ismSTORE_HA_STATE_PRIMARY || !pHAInfo->pAdminChannel)
   {
      TRACE(1, "HAAdmin: Failed to transfer an admin file (Filename %s) because the node state has been changed. State %d, SyncNodesCount %u, pAdminChannel %p\n",
            pFilename, pHAInfo->State, pHAInfo->SyncNodesCount, pHAInfo->pAdminChannel);
      rc = ISMRC_StoreHAError;
      pthread_mutex_unlock(&ismStore_HAAdminMutex);
      goto exit;
   }
   pthread_mutex_unlock(&ismStore_HAAdminMutex);

   if (pHAInfo->AdminFileState != 2)
   {
      TRACE(1, "HAAdmin: Failed to transfer an admin file because of a disk read failure. Filename %s, FileSize %lu, error code %d\n",
            pFilename, diskTask.BufferParams->BufferLength, pHAInfo->AdminFileState);
      rc = ISMRC_StoreDiskError;
      goto exit;
   }

   TRACE(9, "HAAdmin: Start sending an admin file to the Standby node. Filename %s, FileSize %lu\n",
         pFilename, diskTask.BufferParams->BufferLength);

   dataLength = diskTask.BufferParams->BufferLength;
   do
   {
      requiredLength = (dataLength < 1000000000 && dataLength > 0 ? (uint32_t)dataLength : 1000000000);
      if ((ec = ism_store_memHAEnsureBufferAllocation(pHAInfo->pAdminChannel,
                                                      &pBuffer,
                                                      &bufferLength,
                                                      &pPos,
                                                      requiredLength,
                                                      msgType,
                                                      &opcount)) != StoreRC_OK)
      {
         TRACE(1, "HAAdmin: Failed to send an admin file to the Standby node. Filename %s, FileSize %lu, error code %d\n",
               pFilename, diskTask.BufferParams->BufferLength, ec);
         rc = ISMRC_StoreHAError;
         goto exit;
      }

      // Add the header information in the first fragment only
      if (pHAInfo->pAdminChannel->FragSqn == 0)
      {
         ismSTORE_putShort(pPos, Operation_Null);
         plen = strlen(pPath) + 1;
         flen = strlen(pFilename) + 1;
         ismSTORE_putInt(pPos, plen + flen + LONG_SIZE + 2 * SHORT_SIZE);
         ismSTORE_putLong(pPos, diskTask.BufferParams->BufferLength);
         ismSTORE_putShort(pPos, (uint16_t)plen);
         memcpy(pPos, pPath, plen);
         pPos += plen;
         ismSTORE_putShort(pPos, (uint16_t)flen);
         memcpy(pPos, pFilename, flen);
         pPos += flen;
         opcount++;
      }

      fragLength = bufferLength - ((uint32_t)(pPos - pBuffer) + SHORT_SIZE + INT_SIZE + LONG_SIZE);
      if (dataLength < fragLength) {
         fragLength = dataLength;
      }

      ismSTORE_putShort(pPos, Operation_Null);
      ismSTORE_putInt(pPos, fragLength + LONG_SIZE);
      ismSTORE_putLong(pPos, offset);

      if (fragLength > 0)
      {
         memcpy(pPos, diskTask.BufferParams->pBuffer + offset, fragLength);
         pPos += fragLength;
      }
      opcount++;

      dataLength -= fragLength;
      offset += fragLength;
   } while (dataLength > 0);

   // Send the last fragment
   // requiredLength = 0 means that this is the last fragment.
   memset(&ack, '\0', sizeof(ack));
   ack.AckSqn = pHAInfo->pAdminChannel->MsgSqn;
   if ((ec = ism_store_memHAEnsureBufferAllocation(pHAInfo->pAdminChannel, &pBuffer, &bufferLength, &pPos, 0, msgType, &opcount)) != StoreRC_OK)
   {
      TRACE(1, "HAAdmin: Failed to send an admin file to the Standby node. Filename %s, FileSize %lu, error code %d\n",
            pFilename, diskTask.BufferParams->BufferLength, ec);
      rc = ISMRC_StoreHAError;
      goto exit;
   }

   TRACE(9, "HAAdmin: An admin file has been sent. Filename %s, FileSize %lu, MsgSqn %lu, LastFrag %u\n",
         pFilename, diskTask.BufferParams->BufferLength, pHAInfo->pAdminChannel->MsgSqn - 1, pHAInfo->pAdminChannel->FragSqn);

   // Wait for an ACK from the Standby node
   ec = ism_store_memHAReceiveAck(pHAInfo->pAdminChannel, &ack, 0);
   if (ec != StoreRC_OK)
   {
      TRACE(1, "HAAdmin: Failed to receive an ACK for admin file. Filename %s, FileSize %lu, MsgSqn %lu, LastFrag %u, error code %d\n",
            pFilename, diskTask.BufferParams->BufferLength, pHAInfo->pAdminChannel->MsgSqn, pHAInfo->pAdminChannel->FragSqn, ec);
      rc = ISMRC_StoreHAError;
      goto exit;
   }

   if (ack.ReturnCode != StoreRC_OK)
   {
      TRACE(1, "HAAdmin: Failed to process an admin file on the Standby node. Filename %s, FileSize %lu, MsgSqn %lu, LastFrag %u, error code %d\n",
            pFilename, diskTask.BufferParams->BufferLength, pHAInfo->pAdminChannel->MsgSqn, pHAInfo->pAdminChannel->FragSqn, ack.ReturnCode);
      rc = ISMRC_StoreHAError;
      goto exit;
   }

   TRACE(9, "HAAdmin: An admin file has been received by the Standby node. Filename %s, FileSize %lu, MsgSqn %lu, LastFrag %u\n",
         pFilename, diskTask.BufferParams->BufferLength, pHAInfo->pAdminChannel->MsgSqn - 1, pHAInfo->pAdminChannel->FragSqn);

exit:
   if (rc != ISMRC_OK)
   {
      ism_common_setError(rc);
   }

   ism_common_free_raw(ism_memory_store_misc,diskTask.BufferParams->pBuffer);

   pthread_mutex_lock(&ismStore_HAAdminMutex);
   pHAInfo->fAdminChannelBusy = 0;
   pHAInfo->AdminFileState = 0;
   pthread_mutex_unlock(&ismStore_HAAdminMutex);

   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
   return rc;
}

int ism_ha_store_term(void)
{
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   int rc = ISMRC_OK;

   TRACE(9, "Entry: %s\n", __FUNCTION__);
   pthread_mutex_lock(&ismStore_HAAdminMutex);
   if (!ismSTORE_HASSTANDBY)
   {
      TRACE(1, "Failed to make controlled termination of the HA pair, because the node state is not valid. State %d, SyncNodesCount %u\n",
            pHAInfo->State, pHAInfo->SyncNodesCount);
      rc = ISMRC_StoreHAError;
   }
   pthread_mutex_unlock(&ismStore_HAAdminMutex);

   if (rc == ISMRC_OK)
   {
      rc = ism_store_memTerm(1);
   }

   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
   return rc;
}

static void ism_store_memHAAdminDiskReadComplete(ismStore_GenId_t genId, int32_t retcode, ismStore_DiskGenInfo_t *pDiskGenInfo, void *pContext)
{
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   pthread_mutex_lock(&ismStore_HAAdminMutex);
   if (retcode == StoreRC_OK)
   {
      TRACE(7, "HAAdmin: An admin file has been read from the primary disk\n");
      pHAInfo->AdminFileState = 2;
   }
   else
   {
      TRACE(1, "HAAdmin: Failed to read an admin file from the primary disk. error code %d\n", retcode);
      pHAInfo->AdminFileState = 3;
   }
   pthread_mutex_unlock(&ismStore_HAAdminMutex);
}

static void ism_store_memHAAdminDiskWriteComplete(ismStore_GenId_t genId, int32_t retcode, ismStore_DiskGenInfo_t *pDiskGenInfo, void *pContext)
{
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   ismStore_memHADiskWriteCtxt_t *pDiskWriteCtxt = (ismStore_memHADiskWriteCtxt_t *)pContext;
   int ec;

   pthread_mutex_lock(&ismStore_HAAdminMutex);

   if (retcode == StoreRC_OK)
   {
      TRACE(5, "HAAdmin: An admin file has been written to the Standby disk. SrcMsgType %u, AckSqn %lu, ViewId %u, CurrentViewId %u, CurrentRole %u\n",
            pDiskWriteCtxt->Ack.SrcMsgType, pDiskWriteCtxt->Ack.AckSqn, pDiskWriteCtxt->ViewId, pHAInfo->View.ViewId, pHAInfo->View.NewRole);

      pDiskWriteCtxt->Ack.ReturnCode = StoreRC_OK;
   }
   else
   {
      TRACE(1, "HAAdmin: Failed to write an admin file to the Standby disk. SrcMsgType %u, AckSqn %lu, error code %d\n",
            pDiskWriteCtxt->Ack.SrcMsgType, pDiskWriteCtxt->Ack.AckSqn, retcode);
      pDiskWriteCtxt->Ack.ReturnCode = StoreRC_SystemError;
   }

   if ((pHAInfo->State == ismSTORE_HA_STATE_UNSYNC || pHAInfo->State == ismSTORE_HA_STATE_STANDBY) &&
       pDiskWriteCtxt->pHAChannel == pHAInfo->pAdminChannel && !pHAInfo->fAdminTx)
   {
      if ((ec = ism_store_memHASendAck(pDiskWriteCtxt->pHAChannel, &pDiskWriteCtxt->Ack)) != StoreRC_OK)
      {
         TRACE(1, "HAAdmin: Failed to send ACK for an admin file (ChannelId %d, MsgType %u, MsgSqn %lu). error code %d\n",
               pDiskWriteCtxt->pHAChannel->ChannelId, pDiskWriteCtxt->Ack.SrcMsgType, pDiskWriteCtxt->Ack.AckSqn, ec);
      }
   }
   else
   {
      TRACE(9, "HAAdmin: The HA view (ViewId %u) has been changed. No need to send an ACK for admin file. CurrentViewId %u, Role %u, State %u, SrcMsgType %u, AckSqn %lu, ReturnCode %d\n",
            pDiskWriteCtxt->ViewId, pHAInfo->View.ViewId, pHAInfo->View.NewRole, pHAInfo->State, pDiskWriteCtxt->Ack.SrcMsgType, pDiskWriteCtxt->Ack.AckSqn, pDiskWriteCtxt->Ack.ReturnCode);
   }

   ismSTORE_FREE(pDiskWriteCtxt->pArg);
   ismSTORE_FREE(pDiskWriteCtxt);

   pthread_mutex_unlock(&ismStore_HAAdminMutex);
}

/*********************************************************************/
/* Store High-Availability Operations for Thread                     */
/*********************************************************************/
static void ism_store_memHAAddJob(ismStore_memHAJob_t *pJob)
{
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   ismStore_memHAJob_t *pNewJob;

   pNewJob = (ismStore_memHAJob_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,116),sizeof(*pNewJob));
   if (pNewJob)
   {
      memcpy(pNewJob, pJob, sizeof(*pNewJob));
      pNewJob->pNextJob = NULL;
      pthread_mutex_lock(&pHAInfo->ThreadMutex);
      if (pHAInfo->pThreadLastJob)
      {
         pHAInfo->pThreadLastJob->pNextJob = pNewJob;
      }
      else
      {
         pHAInfo->pThreadFirstJob = pNewJob;
      }
      pHAInfo->pThreadLastJob = pNewJob;
      pthread_cond_signal(&pHAInfo->ThreadCond);
      pthread_mutex_unlock(&pHAInfo->ThreadMutex);
      TRACE(9, "Store HA job (Type %u) has been added\n", pJob->JobType);
   }
}

/*
 * Implement the store High-Availability new node synchronization thread.
 *
 * @param arg     Not used
 * @param context Not used
 * @param value   Not used
 * @return NULL
 */
static void *ism_store_memHASyncThread(void *arg, void *pContext, int value)
{
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   ismStore_memHAJob_t *pJob;
   ismStore_memHAMsgType_t msgType = StoreHAMsg_SyncError;
   ismStore_memGenMap_t *pGenMap;
   char threadName[64], *pBuffer=NULL, *pPos;
   uint32_t opcount, bufferLength;
   int rc = StoreRC_OK, ec, i, j;

   memset(threadName, '\0', sizeof(threadName));
   ism_common_getThreadName(threadName, 64);
   TRACE(5, "The %s thread is started\n", threadName);

   pHAInfo->fThreadUp = 1;

   while (rc == StoreRC_OK)
   {
      ism_common_backHome();
      pthread_mutex_lock(&pHAInfo->ThreadMutex);
      while (!(pJob = pHAInfo->pThreadFirstJob) && pHAInfo->fThreadGoOn)
      {
         pthread_cond_wait(&pHAInfo->ThreadCond, &pHAInfo->ThreadMutex);
      }

      if (!pHAInfo->fThreadGoOn)
      {
         pthread_mutex_unlock(&pHAInfo->ThreadMutex);
         break;
      }

      TRACE(9, "The next job of the %s thread is %d\n", threadName, pJob->JobType);
      if (!(pHAInfo->pThreadFirstJob = pJob->pNextJob))
      {
         pHAInfo->pThreadLastJob = NULL;
      }
      pthread_mutex_unlock(&pHAInfo->ThreadMutex);
      ism_common_going2work();
      switch (pJob->JobType)
      {
         case StoreHAJob_SyncList:
            rc = ism_store_memHASyncSendList();
            break;
         case StoreHAJob_SyncDiskGen:
            rc = ism_store_memHASyncSendDiskGen(pJob->GenId);
            break;
         case StoreHAJob_SyncMemGen:
            rc = ism_store_memHASyncSendMemGen();
            break;
         case StoreHAJob_SyncComplete:
            rc = ism_store_memHASyncComplete();
            break;
         case StoreHAJob_SyncAbort:
            rc = StoreRC_SystemError;
            break;
         default:
            TRACE(1, "The job type %d of the %s thread is not valid\n", pJob->JobType, threadName);
      }
      TRACE(9, "The job %d of the %s thread has been completed\n", pJob->JobType, threadName);
      ismSTORE_FREE(pJob);
   }

   TRACE(5, "The %s thread is being stopped\n", threadName);

   if (pHAInfo->pSyncChannel && (rc != StoreRC_OK || pHAInfo->SyncRC != ISMRC_OK))
   {
      if (pHAInfo->SyncRC == ISMRC_OK) { pHAInfo->SyncRC = ISMRC_StoreHAError; }
      TRACE(1, "HASync: Failed to synchronize the Standby node. SyncState 0x%x, error code %d\n", pHAInfo->SyncState, pHAInfo->SyncRC);

      // Send a SyncError message to the remote node
      if (ism_store_memHAEnsureBufferAllocation(pHAInfo->pSyncChannel, &pBuffer, &bufferLength, &pPos, 64, msgType, &opcount) == StoreRC_OK)
      {
         ismSTORE_putShort(pPos, Operation_Null);
         ismSTORE_putInt(pPos, INT_SIZE);
         ismSTORE_putInt(pPos, pHAInfo->SyncRC);
         opcount++;

         // Send the last fragment
         // requiredLength = 0 means that this is the last fragment.
         if (ism_store_memHAEnsureBufferAllocation(pHAInfo->pSyncChannel, &pBuffer, &bufferLength, &pPos, 0, msgType, &opcount) == StoreRC_OK)
         {
            TRACE(9, "HASync: A message (ChannelId %d, MsgType %u, MsgSqn %lu, LastFrag %u, AckSqn %lu) has been sent\n",
                  pHAInfo->pSyncChannel->ChannelId, msgType, pHAInfo->pSyncChannel->MsgSqn - 1, pHAInfo->pSyncChannel->FragSqn, pHAInfo->pSyncChannel->AckSqn);
         }
      }
   }

   // Make sure that there are no generation files in the middle of disk read
   pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);
   for (i=0; i < ismStore_memGlobal.GensMapSize; i++)
   {
      if ((pGenMap = ismStore_memGlobal.pGensMap[i]))
      {
         for (j=0; j < 10000 && pGenMap->HASyncState == StoreHASyncGen_Reading && ismStore_memGlobal.State != ismSTORE_STATE_TERMINATING; j++)
         {
            pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
            if ((j > 0) && (j % 1000 == 0))
            {
               TRACE(4, "HASync: The %s thread is still waiting for a disk read of generation file (GenId %u)\n", threadName, i);
            }
            ism_common_sleep(100000);
            pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);
         }
         if (pGenMap->HASyncState == StoreHASyncGen_Reading)
         {
            TRACE(1, "HASync: Generation file (GenId %u) is still being read from the Primary disk\n", i);
         }
         else
         {
            pGenMap->HASyncBufferLength = 0;
            pGenMap->HASyncDataLength = 0;
            ismSTORE_FREE(pGenMap->pHASyncBuffer);
         }
         pGenMap->HASyncState = StoreHASyncGen_Empty;
      }
   }
   pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);

   pthread_mutex_lock(&pHAInfo->Mutex);
   if (pHAInfo->pSyncChannel)
   {
      ism_store_memHACloseChannel(pHAInfo->pSyncChannel, 1);
      pHAInfo->pSyncChannel = NULL;
   }

   while ((pJob = (ismStore_memHAJob_t *)pHAInfo->pThreadFirstJob))
   {
      if (!(pHAInfo->pThreadFirstJob = pJob->pNextJob))
      {
         pHAInfo->pThreadLastJob = NULL;
      }
      ismSTORE_FREE(pJob);
   }

   if (pHAInfo->fSyncLocked)
   {
     ism_store_memUnlockStore(LOCKSTORE_CALLER_SYNC);
     pHAInfo->fSyncLocked = 0;
   }
   pHAInfo->SyncState = 0;
   pHAInfo->SyncSentBytes = 0;
   pHAInfo->SyncExpGensCount = pHAInfo->SyncSentGensCount = 0;
   pHAInfo->fThreadUp = 0;
   pHAInfo->fThreadGoOn = 0;
   pthread_mutex_unlock(&pHAInfo->Mutex);

   if ((ec = ism_common_detachThread(ism_common_threadSelf())) != 0)
   {
      TRACE(3, "Failed to detach the %s thread. error code %d\n", threadName, ec);
   }
   TRACE(5, "The %s thread is stopped\n", threadName);
   //ism_common_endThread(NULL);
   return NULL;
}

static void ism_store_memHASetHasStandby(void)
{
  if ( ismStore_memGlobal.fEnablePersist ) ism_storePersist_wrLock();
  ismStore_memGlobal.HAInfo.HasStandby = (ismStore_memGlobal.HAInfo.State == ismSTORE_HA_STATE_PRIMARY && ismStore_memGlobal.HAInfo.SyncNodesCount > 1) ; 
  if ( ismStore_memGlobal.fEnablePersist ) ism_storePersist_wrUnlock();
}

/*********************************************************************/
/*                                                                   */
/* End of storeMemoryHA.c                                            */
/*                                                                   */
/*********************************************************************/
