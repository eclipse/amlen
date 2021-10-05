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
/* Module Name: store.c                                              */
/*                                                                   */
/* Description: Main module for Store Component of ISM               */
/*                                                                   */
/*********************************************************************/
#define TRACE_COMP Store
#include <stdio.h>
#include <ismutil.h>
#include <ha.h>
#include <config.h>
#include "storeInternal.h"

/*********************************************************************/
/* Global data for the messaging server. Used to anchor server-wide  */
/* data structures.                                                  */
/*********************************************************************/
ismStore_global_t ismStore_global =
{
    ismSTORE_GLOBAL_STRUCID,
    ismSTORE_MEMTYPE_SHM,
    ismSTORE_CACHEFLUSH_NORMAL,
    ismSTORE_CFG_COLD_START_DV,
    ismSTORE_CFG_RESTORE_DISK_DV,
    ismSTORE_CFG_DISK_CLEAR_DV,
    ismHA_CFG_ENABLEHA_DV,
};

static pthread_mutex_t ismStore_Mutex = PTHREAD_MUTEX_INITIALIZER;
       pthread_mutex_t ismStore_FnMutex = PTHREAD_MUTEX_INITIALIZER;

extern int ism_store_initHAConfig(void);

/*********************************************************************/
/* Store Init                                                        */
/*                                                                   */
/* Performs store init tasks                                         */
/*                                                                   */
/* @param pParams       Store parameters                             */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int ism_store_init(void)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s\n", __FUNCTION__);

   pthread_mutex_lock(&ismStore_Mutex);

   rc = ism_store_initHAConfig();
   ismStore_global.MemType = ism_common_getIntConfig(ismSTORE_CFG_MEMTYPE, ismSTORE_CFG_MEMTYPE_DV);
   ismStore_global.CacheFlushMode = ism_common_getIntConfig(ismSTORE_CFG_CACHEFLUSH_MODE, ismSTORE_CFG_CACHEFLUSH_MODE_DV);
   ismStore_global.ColdStartMode = ism_common_getIntConfig(ismSTORE_CFG_COLD_START, ismSTORE_CFG_COLD_START_DV);
   ismStore_global.fRestoreFromDisk = ism_common_getBooleanConfig(ismSTORE_CFG_RESTORE_DISK, ismSTORE_CFG_RESTORE_DISK_DV);
   ismStore_global.fClearStoredFiles = ism_common_getBooleanConfig(ismSTORE_CFG_DISK_CLEAR, ismSTORE_CFG_DISK_CLEAR_DV);
   ismStore_global.fHAEnabled = ism_common_getBooleanConfig(ismHA_CFG_ENABLEHA, ismHA_CFG_ENABLEHA_DV);

   TRACE(5, "Store parameter %s %u\n", ismSTORE_CFG_MEMTYPE,         ismStore_global.MemType);
   TRACE(5, "Store parameter %s %u\n", ismSTORE_CFG_CACHEFLUSH_MODE, ismStore_global.CacheFlushMode);
   TRACE(5, "Store parameter %s %d\n", ismSTORE_CFG_COLD_START,      ismStore_global.ColdStartMode);
   TRACE(5, "Store parameter %s %d\n", ismSTORE_CFG_RESTORE_DISK,    ismStore_global.fRestoreFromDisk);
   TRACE(5, "Store parameter %s %d\n", ismSTORE_CFG_DISK_CLEAR,      ismStore_global.fClearStoredFiles);
   TRACE(5, "Store parameter %s %d\n", ismHA_CFG_ENABLEHA,           ismStore_global.fHAEnabled);

   ismStore_global.fClearStoredFiles *= ismStore_global.ColdStartMode; // clear files is only relevant in cold start mode

   if (ismStore_global.MemType != ismSTORE_MEMTYPE_SHM && ismStore_global.MemType != ismSTORE_MEMTYPE_NVRAM && ismStore_global.MemType != ismSTORE_MEMTYPE_MEM)
   {
      TRACE(1, "Store parameter %s (%u) is not valid. Valid range: [%u, %u]\n",
            ismSTORE_CFG_MEMTYPE, ismStore_global.MemType, ismSTORE_MEMTYPE_SHM, ismSTORE_MEMTYPE_MEM);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_MEMTYPE, ismStore_global.MemType);
   }

   if (ismStore_global.CacheFlushMode != ismSTORE_CACHEFLUSH_NORMAL && ismStore_global.CacheFlushMode != ismSTORE_CACHEFLUSH_ADR)
   {
      TRACE(1, "Store parameter %s (%u) is not valid. Valid range: [%u, %u]\n",
            ismSTORE_CFG_CACHEFLUSH_MODE, ismStore_global.CacheFlushMode, ismSTORE_CACHEFLUSH_NORMAL, ismSTORE_CACHEFLUSH_ADR);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_CACHEFLUSH_MODE, ismStore_global.CacheFlushMode);
   }

   if (ismStore_global.fRestoreFromDisk && ((ismStore_global.ColdStartMode > 0) || ismStore_global.fClearStoredFiles))
   {
      TRACE(1, "Store parameter %s (%u) is not valid, because there is a conflict with parameters %s (%u) and %s (%u)\n",
            ismSTORE_CFG_RESTORE_DISK, ismStore_global.fRestoreFromDisk, ismSTORE_CFG_COLD_START, ismStore_global.ColdStartMode, ismSTORE_CFG_DISK_CLEAR, ismStore_global.fClearStoredFiles);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_RESTORE_DISK, ismStore_global.fRestoreFromDisk);
   }

   if (rc == ISMRC_OK)
   {
      if (ismStore_global.ColdStartMode == 1 && ismStore_global.fHAEnabled)
      {
         ismStore_global.fHAEnabled = 0;
         TRACE(5, "Store High-Availability has been turned off for store cleanup\n");
      }
      rc = ism_store_memInit();
   }

   pthread_mutex_unlock(&ismStore_Mutex);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

/*********************************************************************/
/* Store Start                                                       */
/*                                                                   */
/* Performs startup tasks.                                           */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int ism_store_start(void)
{
   int32_t rc = ISMRC_OK;
   int count=2 ; 
   int resync=0 ; 

   TRACE(9, "Entry: %s\n", __FUNCTION__);
 restart:
   pthread_mutex_lock(&ismStore_Mutex);

   ismStore_global.PrimaryMemSizeBytes = 0;
   ismStore_global.fSyncFailed = 0 ; 
   rc = (ismStore_global.pFnStart ? (*ismStore_global.pFnStart)() : ISMRC_StoreNotAvailable);

   pthread_mutex_unlock(&ismStore_Mutex);

   if ( rc != ISMRC_OK && ismStore_global.PrimaryMemSizeBytes && count )
   {
     ism_field_t f;
     ism_prop_t * props = ism_common_getConfigProperties() ;
     TRACE(1,"Failed to establish HA-Pair due to store size mismatch.  Will adjust local store size to %lu and restart store...\n",ismStore_global.PrimaryMemSizeBytes);
     ism_store_term();
     f.type = VT_ULong;
     f.val.l = ismStore_global.PrimaryMemSizeBytes ; 
     ism_common_setProperty( props, ismSTORE_CFG_TOTAL_MEMSIZE_BYTES, &f);
     f.type = VT_UInt ;
     f.val.u = 2 ; 
     ism_common_setProperty( props, ismSTORE_CFG_COLD_START      , &f);
     ism_store_init();
     count-- ; 
     ismStore_global.fRestarting = 1;
     goto restart ; 
   }
   if ( rc != ISMRC_OK && ismStore_global.fSyncFailed )
   {
     if ( resync < ism_common_getIntConfig(ismHA_CFG_NUMRESYNCATTEMPS,5) )
     {
       resync++ ; 
       TRACE(1,"Failed to establish HA-Pair due to a failure to sync as SB.  Will cross fingers and restart store for the %u time...\n",resync);
       ism_store_term();
       ism_store_init();
       goto restart ; 
     }
     if ( ism_common_getIntConfig(ismHA_CFG_SYNCPOLICY,0) )
     {
       TRACE(1,"Failed to establish HA-Pair due to a failure to sync as SB.  Will shutdown after restarting for %u times!!!\n",resync);
       ism_common_shutdown(0);
     }
   } 

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

/*********************************************************************/
/* Store Shutdown                                                    */
/*                                                                   */
/* Performs shutdown tasks.                                          */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int ism_store_term(void)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s\n", __FUNCTION__);

   pthread_mutex_lock(&ismStore_Mutex);

   rc = (ismStore_global.pFnTerm ? (*ismStore_global.pFnTerm)(0) : ISMRC_StoreNotAvailable);

   pthread_mutex_unlock(&ismStore_Mutex);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

/*********************************************************************/
/* Store Shutdown that includes terminating the HA standby           */
/*                                                                   */
/* Performs shutdown tasks.                                          */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int ism_store_termHA(void)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s\n", __FUNCTION__);

   pthread_mutex_lock(&ismStore_Mutex);

   rc = (ismStore_global.pFnTerm ? (*ismStore_global.pFnTerm)(1) : ISMRC_StoreNotAvailable);

   pthread_mutex_unlock(&ismStore_Mutex);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

/*********************************************************************/
/* Store Drop                                                        */
/*                                                                   */
/* Drops the store.                                                  */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int ism_store_drop(void)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s\n", __FUNCTION__);

   pthread_mutex_lock(&ismStore_Mutex);

   rc = (ismStore_global.pFnDrop ? (*ismStore_global.pFnDrop)() : ISMRC_StoreNotAvailable);

   pthread_mutex_unlock(&ismStore_Mutex);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

/*********************************************************************/
/* Store Event Registration                                          */
/*                                                                   */
/* Registers event callback.                                         */
/* @param callback      Callback function                            */
/* @param pContext      Context to associate with the callback       */
/*                      function                                     */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int ism_store_registerEventCallback(ismPSTOREEVENT callback, void *pContext)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s\n", __FUNCTION__);

   pthread_mutex_lock(&ismStore_Mutex);

   rc = (ismStore_global.pFnRegisterEventCallback ? (*ismStore_global.pFnRegisterEventCallback)(callback, pContext) : ISMRC_StoreNotAvailable);

   pthread_mutex_unlock(&ismStore_Mutex);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

/*********************************************************************/
/* Get Generation ID of a store handle                               */
/*                                                                   */
/* Returns the generation ID of a store handle.                      */
/*                                                                   */
/* @param handle        Store handle                                 */
/* @param pGenId        Pointer to the generation id                 */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_getGenIdOfHandle(ismStore_Handle_t handle, ismStore_GenId_t *pGenId)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s. Handle 0x%lx\n", __FUNCTION__, handle);

   rc = (*ismStore_global.pFnGetGenIdOfHandle)(handle, pGenId);
   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. GenId %d, rc %d\n", __FUNCTION__, (pGenId ? *pGenId : -1), rc);

   return rc;
}

/*********************************************************************/
/* Get Store Statistics                                              */
/*                                                                   */
/* Returns the store statistics.                                     */
/*                                                                   */
/* @param pStatistics     Pointer to the statistics structure        */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_getStatistics(ismStore_Statistics_t *pStatistics)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s\n", __FUNCTION__);

   pthread_mutex_lock(&ismStore_FnMutex);

   rc = (ismStore_global.pFnGetStatistics ? (*ismStore_global.pFnGetStatistics)(pStatistics) : ISMRC_StoreNotAvailable);

   pthread_mutex_unlock(&ismStore_FnMutex);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

/*********************************************************************/
/* Open Stream                                                       */
/*                                                                   */
/* Opens a stream. All data access operations are part of a          */
/* store-transaction, and a stream is a sequence of                  */
/* store-transactions.                                               */
/*                                                                   */
/* @param phStream      Handle of newly opened stream                */
/* @param fHighPerf     Indicates if the stream is high-performance  */
/*                      or not                                       */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_openStream(ismStore_StreamHandle_t *phStream, uint8_t fHighPerf)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s. fHighPerf %u\n", __FUNCTION__, fHighPerf);

   pthread_mutex_lock(&ismStore_Mutex);

   rc = (ismStore_global.pFnOpenStream ? (*ismStore_global.pFnOpenStream)(phStream, fHighPerf) : ISMRC_StoreNotAvailable);

   pthread_mutex_unlock(&ismStore_Mutex);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. hStream %d, rc %d\n", __FUNCTION__, (phStream ? *phStream : -1), rc);

   return rc;
}

/*********************************************************************/
/* Close Stream                                                      */
/*                                                                   */
/* Closes a stream. Any in-progress store-transaction is rolled      */
/* back.                                                             */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_closeStream(ismStore_StreamHandle_t hStream)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s. hStream %u\n", __FUNCTION__, hStream);

   pthread_mutex_lock(&ismStore_Mutex);

   rc = (ismStore_global.pFnCloseStream ? (*ismStore_global.pFnCloseStream)(hStream) : ISMRC_StoreNotAvailable);

   pthread_mutex_unlock(&ismStore_Mutex);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

/*********************************************************************/
/* Reserve resources for a Stream                                    */
/*                                                                   */
/* Ensures that the Stream has enough resources to perform a         */
/* store-transaction without filling up the generation.              */
/* The reservation structure defines the resources that should be    */
/* reserved.                                                         */
/* This API must be called as the first operation of a               */
/* store-transaction, otherwise an ISMRC_StoreTransActive error code */
/* is returned.                                                      */
/*                                                                   */
/* @param hStream       Stream handle                                */
/* @param pReservation  Pointer to a reservation structure           */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_reserveStreamResources(ismStore_StreamHandle_t hStream,
                                              ismStore_Reservation_t *pReservation)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s. hStream %u, DataLength %lu, RecordsCount %u, RefsCount %u\n",
         __FUNCTION__, hStream, (pReservation ? pReservation->DataLength : 0),
         (pReservation ? pReservation->RecordsCount : 0), (pReservation ? pReservation->RefsCount : 0));

   rc = (ismStore_global.pFnReserveStreamResources ?
                 (*ismStore_global.pFnReserveStreamResources)(hStream, pReservation) :
                 ISMRC_StoreNotAvailable);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

/*********************************************************************/
/* Cancel resources reservation for a Stream                         */
/*                                                                   */
/* This function is used to cancel a reservation that was made using */
/* ism_store_reserveStreamResources and to complete the store-       */
/* transaction that this call initiated.                             */
/*                                                                   */
/* This API must be called as the second operation of a store-       */
/* transaction following a call to ism_store_reserveStreamResources. */
/* If any other operations have been performed as part of the store- */
/* transaction an ISMRC_StoreTransActive error code is returned.     */
/*                                                                   */
/* @param hStream       Stream handle                                */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_cancelResourceReservation(ismStore_StreamHandle_t hStream)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s. hStream %u\n", __FUNCTION__, hStream);

   rc = (ismStore_global.pFnCancelResourceReservation ?
                 (*ismStore_global.pFnCancelResourceReservation)(hStream) :
                 ISMRC_StoreNotAvailable);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}


/*********************************************************************/
/* Starts a store-transaction if one is not already started.         */
/* The function does nothing (and returns OK) if a store-transaction */
/* was already active when it was called.                            */
/*                                                                   */
/* @param hStream       Stream handle                                */
/* @param fIsActive     An output flag indicating if a store-        */
/*                      transaction was already active               */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_startTransaction(ismStore_StreamHandle_t hStream, int *fIsActive)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s. hStream %u\n", __FUNCTION__, hStream);

   rc = (ismStore_global.pFnStartTransaction ?
                 (*ismStore_global.pFnStartTransaction)(hStream,fIsActive) :
                 ISMRC_StoreNotAvailable);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}


/*********************************************************************/
/* Cancel a store-transaction that was started using                 */
/* ism_store_startTransaction().                                     */
/*                                                                   */
/* This API must be called as the second operation of a store-       */
/* transaction following a call to ism_store_startTransaction.       */
/* If any other operations have been performed as part of the store- */
/* transaction an ISMRC_StoreTransActive error code is returned.     */
/*                                                                   */
/* @param hStream       Stream handle                                */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_cancelTransaction(ismStore_StreamHandle_t hStream)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s. hStream %u\n", __FUNCTION__, hStream);

   rc = (ismStore_global.pFnCancelTransaction ?
                 (*ismStore_global.pFnCancelTransaction)(hStream) :
                 ISMRC_StoreNotAvailable);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}


/*********************************************************************/
/* Get the number of un-committed operations on the Stream           */
/*                                                                   */
/* @param hStream       Stream handle                                */
/* @param pOpsCount     Pointer to the stream operations count       */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_getStreamOpsCount(ismStore_StreamHandle_t hStream,
                                         uint32_t *pOpsCount)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s. hStream %u\n", __FUNCTION__, hStream);

   rc = (ismStore_global.pFnGetStreamOpsCount ?
                 (*ismStore_global.pFnGetStreamOpsCount)(hStream, pOpsCount) :
                 ISMRC_StoreNotAvailable);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. opsCount %d, rc %d\n", __FUNCTION__, (pOpsCount ? *pOpsCount : -1), rc);

   return rc;
}

/*********************************************************************/
/* Open Reference Context                                            */
/*                                                                   */
/* Opens a reference context. This is used to manage the creation    */
/* and deletion of references for a specific owner.                  */
/*                                                                   */
/* @param hOwnerHandle  Store handle of owner                        */
/* @param pRefStats     Pointer to a structure where the references  */
/*                      statistics will be copied (output).          */
/* @param phRefCtxt     Handle of newly opened reference context     */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_openReferenceContext(ismStore_Handle_t hOwnerHandle,
                                            ismStore_ReferenceStatistics_t *pRefStats,
                                            void **phRefCtxt)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s. hOwnerHandle 0x%lx\n", __FUNCTION__, hOwnerHandle);

   rc = (ismStore_global.pFnOpenReferenceContext ?
                 (*ismStore_global.pFnOpenReferenceContext)(hOwnerHandle, pRefStats, phRefCtxt) :
                 ISMRC_StoreNotAvailable);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

/*********************************************************************/
/* Close Reference Context                                           */
/*                                                                   */
/* Closes a reference context.                                       */
/*                                                                   */
/* @param hRefCtxt      Reference context handle                     */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_closeReferenceContext(void *hRefCtxt)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s\n", __FUNCTION__);

   rc = (ismStore_global.pFnCloseReferenceContext ?
                 (*ismStore_global.pFnCloseReferenceContext)(hRefCtxt) :
                 ISMRC_StoreNotAvailable);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

#if 0
/*********************************************************************/
/* Clear Reference Context                                           */
/*                                                                   */
/* Clears the content of the reference context (including the        */
/* references) as part of a store-transaction.                       */
/*                                                                   */
/* @param hStream       Stream handle                                */
/* @param hRefCtxt      Reference context handle                     */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_clearReferenceContext(ismStore_StreamHandle_t hStream,
                                             void *hRefCtxt)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s. hStream %u\n", __FUNCTION__, hStream);

   rc = (*ismStore_global.pFnClearReferenceContext)(hStream, hRefCtxt);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}
#endif

/*********************************************************************/
/* Open State Context                                                */
/*                                                                   */
/* Opens a state context. This is used to manage the creation and    */
/* deletion of states for a specific owner.                          */
/*                                                                   */
/* @param hOwnerHandle  Store handle of owner                        */
/* @param phStateCtxt   Handle of newly opened state context         */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_openStateContext(ismStore_Handle_t hOwnerHandle, void **phStateCtxt)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s. hOwnerHandle 0x%lx\n", __FUNCTION__, hOwnerHandle);

   rc = (ismStore_global.pFnOpenStateContext ?
                 (*ismStore_global.pFnOpenStateContext)(hOwnerHandle, phStateCtxt) :
                 ISMRC_StoreNotAvailable);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

/*********************************************************************/
/* Close State Context                                               */
/*                                                                   */
/* Closes a state context.                                           */
/*                                                                   */
/* @param hStateCtxt    State context handle                         */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_closeStateContext(void *hStateCtxt)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s\n", __FUNCTION__);

   rc = (ismStore_global.pFnCloseStateContext ? (*ismStore_global.pFnCloseStateContext)(hStateCtxt) : ISMRC_StoreNotAvailable);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

/*********************************************************************/
/* Commit Store-Transaction                                          */
/*                                                                   */
/* Commits a store-transaction. The store-transactions committed to  */
/* a stream are applied to the store in order.                       */
/*                                                                   */
/* @param hStream       Stream handle                                */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_commit(ismStore_StreamHandle_t hStream)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s. hStream %u\n", __FUNCTION__, hStream);

   rc = (*ismStore_global.pFnEndStoreTransaction)(hStream, 1, NULL, NULL);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

/*********************************************************************/
/* Asynchronous commit of a Store-Transaction                        */
/*                                                                   */
/* Commits a store-transaction. The store-transactions committed to  */
/* a stream are applied to the store in order.                       */
/*                                                                   */
/* @param hStream       Stream handle                                */
/* @param pCallback     Completion callback to be invoked when the   */
/*                      Store transaction is committed.              */
/* @param pContext      User context to associate with the operation */
/*                                                                   */
/* @return ISMRC_AsyncCompletion on successful completion or an      */
/*                      ISMRC_ value.                                */
/*                                                                   */
/* @remark  Unless an error is detected early the operation always   */
/*          completes asynchronously and invikes the callback        */
/*********************************************************************/
XAPI int32_t ism_store_asyncCommit(ismStore_StreamHandle_t hStream, 
                                   ismStore_CompletionCallback_t pCallback,
                                   void *pContext) 
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s. hStream %u, pCallback %p, pContext %p\n", __FUNCTION__, hStream,pCallback,pContext);

   rc = (*ismStore_global.pFnEndStoreTransaction)(hStream, 1, pCallback, pContext);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

/*********************************************************************/
/* Rollback Store-Transaction                                        */
/*                                                                   */
/* Rolls back a store-transaction.                                   */
/*                                                                   */
/* @param hStream       Stream handle                                */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_rollback(ismStore_StreamHandle_t hStream)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s. hStream %u\n", __FUNCTION__, hStream);

   rc = (*ismStore_global.pFnEndStoreTransaction)(hStream, 0, NULL, NULL);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

/*********************************************************************/
/* Create Record definition                                          */
/*                                                                   */
/* Creates a Record Definition in the Store.                         */
/*                                                                   */
/* @param hStream       Stream handle                                */
/* @param pRecord       Pointer to the record structure              */
/* @param pHandle       Store handle of created record               */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_createRecord(ismStore_StreamHandle_t hStream,
                                    const ismStore_Record_t *pRecord,
                                    ismStore_Handle_t *pHandle)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s. hStream %u, RecordType 0x%x, DataLength %u\n",
         __FUNCTION__, hStream, pRecord->Type, pRecord->DataLength);

   rc = (*ismStore_global.pFnAssignRecordAllocation)(hStream, pRecord, pHandle);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. Handle 0x%lx, rc %d\n", __FUNCTION__, (pHandle ? *pHandle : 0), rc);

   return rc;
}

/*********************************************************************/
/* Update Record definition                                          */
/*                                                                   */
/* Updates a Record Definition in the Store.                         */
/*                                                                   */
/* NOTE: Only records that are stored in the store memory can be     */
/* updated using this API. (i.e., records of type                    */
/* ISM_STORE_RECTYPE_MSG could NOT be updated).                      */
/*                                                                   */
/* @param hStream       Stream handle                                */
/* @param handle        Store record handle                          */
/* @param attribute     new attribute of the record                  */
/* @param state         new state of the record                      */
/* @param flags         Indicates the fields to be updated using     */
/*                      bitwise-or:                                  */
/*                      ismSTORE_SET_ATTRIBUTE - Set attribute field */
/*                      ismSTORE_SET_STATE     - Set state field     */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_updateRecord(ismStore_StreamHandle_t hStream,
                                    ismStore_Handle_t handle,
                                    uint64_t attribute,
                                    uint64_t state,
                                    uint8_t  flags)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s. hStream %u, Handle 0x%lx, Attribute %lu, State %lu, Flags 0x%x\n",
         __FUNCTION__, hStream, handle, attribute, state, flags);

   rc = (*ismStore_global.pFnUpdateRecord)(hStream, handle, attribute, state, flags);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

/*********************************************************************/
/* Delete Record Definition                                          */
/*                                                                   */
/* Deletes a Record Definition in the Store.                         */
/* All the reference chunks and state objects that are linked (by    */
/* owner) of this record are also deleted.                           */
/*                                                                   */
/* @param hStream       Stream handle                                */
/* @param handle        Store record handle                          */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_deleteRecord(ismStore_StreamHandle_t hStream,
                                    ismStore_Handle_t handle)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s. hStream %u, Handle 0x%lx\n", __FUNCTION__, hStream, handle);

   rc = (*ismStore_global.pFnFreeRecordAllocation)(hStream, handle);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

/*********************************************************************/
/* Create Reference                                                  */
/*                                                                   */
/* Creates a Reference from one handle to another in the Store.      */
/* The references are stored in order and best performance is        */
/* achieved if they are created in order, or approximately in order. */
/*                                                                   */
/* @param hStream       Stream handle                                */
/* @param hRefCtxt      Reference context handle                     */
/* @param pRef          Pointer to the reference structure           */
/* @param minimumActiveOrderId   Minimum order id for references     */
/* @param pHandle       Store handle of created reference            */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_createReference(ismStore_StreamHandle_t hStream,
                                       void *hRefCtxt,
                                       const ismStore_Reference_t *pRef,
                                       uint64_t minimumActiveOrderId,
                                       ismStore_Handle_t *pHandle)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s. hStream %u, OrderId %lu, Value %u, State %u, hRefHandle 0x%lx, MinActiveOrderId %lu\n",
         __FUNCTION__, hStream, pRef->OrderId, pRef->Value, pRef->State, pRef->hRefHandle, minimumActiveOrderId);

   rc = (*ismStore_global.pFnAddReference)(hStream, hRefCtxt, pRef, minimumActiveOrderId, 0, pHandle);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. Handle 0x%lx, rc %d\n", __FUNCTION__, (pHandle ? *pHandle : 0), rc);

   return rc;
}

XAPI int32_t ism_store_createReferenceCommit(
                                       ismStore_StreamHandle_t hStream,
                                       void *hRefCtxt,
                                       const ismStore_Reference_t *pRef,
                                       uint64_t minimumActiveOrderId,
                                       ismStore_Handle_t *pHandle)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s. hStream %u, OrderId %lu, Value %u, State %u, hRefHandle 0x%lx, MinActiveOrderId %lu\n",
         __FUNCTION__, hStream, pRef->OrderId, pRef->Value, pRef->State, pRef->hRefHandle, minimumActiveOrderId);

   rc = (*ismStore_global.pFnAddReference)(hStream, hRefCtxt, pRef, minimumActiveOrderId, 1, pHandle);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. Handle 0x%lx, rc %d\n", __FUNCTION__, (pHandle ? *pHandle : 0), rc);

   return rc;
}

/*********************************************************************/
/* Update Reference                                                  */
/*                                                                   */
/* Updates a Reference from one handle to another in the Store.      */
/*                                                                   */
/* @param hStream       Stream handle                                */
/* @param hRefCtxt      Reference context handle                     */
/* @param handle        Store handle of the reference                */
/* @param orderId       Order Id of the reference (required only for */
/*                      references that are stored on the disk)      */
/* @param state         New state of the reference                   */
/* @param minimumActiveOrderId   Minimum order id for references     */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_updateReference(ismStore_StreamHandle_t hStream,
                                       void *hRefCtxt,
                                       ismStore_Handle_t handle,
                                       uint64_t orderId,
                                       uint8_t state,
                                       uint64_t minimumActiveOrderId)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s. hStream %u, Handle 0x%lx, OrderId %lu, State %u, MinActiveOrderId %lu\n",
         __FUNCTION__, hStream, handle, orderId, state, minimumActiveOrderId);

   rc = (*ismStore_global.pFnUpdateReference)(hStream,
                                              hRefCtxt,
                                              handle,
                                              orderId,
                                              state,
                                              minimumActiveOrderId,
                                              0);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

XAPI int32_t ism_store_updateReferenceCommit(
                                       ismStore_StreamHandle_t hStream,
                                       void *hRefCtxt,
                                       ismStore_Handle_t handle,
                                       uint64_t orderId,
                                       uint8_t state,
                                       uint64_t minimumActiveOrderId)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s. hStream %u, Handle 0x%lx, OrderId %lu, State %u, MinActiveOrderId %lu\n",
         __FUNCTION__, hStream, handle, orderId, state, minimumActiveOrderId);

   rc = (*ismStore_global.pFnUpdateReference)(hStream,
                                              hRefCtxt,
                                              handle,
                                              orderId,
                                              state,
                                              minimumActiveOrderId,
                                              1);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

/*********************************************************************/
/* Delete Reference                                                  */
/*                                                                   */
/* Deletes a Reference from one handle to another in the Store.      */
/* The caller must ensure that the referred-to record is not         */
/* orphaned.                                                         */
/*                                                                   */
/* @param hStream       Stream handle                                */
/* @param hRefCtxt      Reference context handle                     */
/* @param handle        Store handle of the reference                */
/* @param orderId       Order Id of the reference (required only for */
/*                      references that are stored on the disk)      */
/* @param minimumActiveOrderId   Minimum order id for references     */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_deleteReference(ismStore_StreamHandle_t hStream,
                                       void *hRefCtxt,
                                       ismStore_Handle_t handle,
                                       uint64_t orderId,
                                       uint64_t minimumActiveOrderId)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s. hStream %u, Handle 0x%lx, OrderId %lu, MinActiveOrderId %lu\n",
         __FUNCTION__, hStream, handle, orderId, minimumActiveOrderId);

   rc = (*ismStore_global.pFnDeleteReference)(hStream, hRefCtxt, handle, orderId, minimumActiveOrderId, 0);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

XAPI int32_t ism_store_deleteReferenceCommit(
                                       ismStore_StreamHandle_t hStream,
                                       void *hRefCtxt,
                                       ismStore_Handle_t handle,
                                       uint64_t orderId,
                                       uint64_t minimumActiveOrderId)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s. hStream %u, Handle 0x%lx, OrderId %lu, MinActiveOrderId %lu\n",
         __FUNCTION__, hStream, handle, orderId, minimumActiveOrderId);

   rc = (*ismStore_global.pFnDeleteReference)(hStream, hRefCtxt, handle, orderId, minimumActiveOrderId, 1);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

/*********************************************************************/
/* Prune References                                                  */
/*                                                                   */
/* Prunes references of an owner in the Store.                       */
/*                                                                   */
/* @param hRefCtxt      Reference context handle                     */
/* @param minimumActiveOrderId   Minimum order id for references     */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_pruneReferences(void *hRefCtxt,
                                       uint64_t minimumActiveOrderId)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s. MinActiveOrderId %lu\n", __FUNCTION__, minimumActiveOrderId);

   rc = (*ismStore_global.pFnPruneReferences)((ismStore_StreamHandle_t)(-1), hRefCtxt, minimumActiveOrderId);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}


/*********************************************************************/
/* Set the minimum active order ID for an owner                      */
/*                                                                   */
/* Setting minActiveOrderId is used to allow the Store to prune      */
/* references of an owner in order to free resources.                */
/* Setting minActiveOrderId is not performed as a store-transaction  */
/* and is not affected by commit/rollback. It can be invoked at any  */
/* time while a store-transaction is in progress.                    */
/* Setting minActiveOrderId does NOT guarantee that all the          */
/* references below the provided minimumActiveOrderId are deleted    */
/* once the function returns.                                        */
/*                                                                   */
/* @param hStream       Stream handle                                */
/* @param hRefCtxt      Reference context handle                     */
/* @param minimumActiveOrderId   Minimum order id for references     */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_setMinActiveOrderId(ismStore_StreamHandle_t hStream,
                                       void *hRefCtxt,
                                       uint64_t minimumActiveOrderId) 
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s. hStream %u, MinActiveOrderId %lu\n", __FUNCTION__, hStream, minimumActiveOrderId);

   rc = (*ismStore_global.pFnPruneReferences)(hStream, hRefCtxt, minimumActiveOrderId);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

/*********************************************************************/
/* Create State                                                      */
/*                                                                   */
/* Creates a State object for a specified record in the Store.       */
/* The state objects are stored in order and best performance is     */
/* achieved if they are deleted in order, or approximately in order. */
/*                                                                   */
/* @param hStream       Stream handle                                */
/* @param hStateCtxt    State context handle                         */
/* @param pState        Pointer to the state object structure        */
/* @param pHandle       Store handle of created state                */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_createState(ismStore_StreamHandle_t hStream,
                                   void *hStateCtxt,
                                   const ismStore_StateObject_t *pState,
                                   ismStore_Handle_t *pHandle)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s. hStream %u, Value %u\n", __FUNCTION__, hStream, pState->Value);

   rc = (*ismStore_global.pFnAddState)(hStream, hStateCtxt, pState, pHandle);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. Handle 0x%lx, rc %d\n", __FUNCTION__, (pHandle ? *pHandle : 0), rc);

   return rc;
}

/*********************************************************************/
/* Delete State                                                      */
/*                                                                   */
/* Deletes a State object for a specified record in the Store.       */
/*                                                                   */
/* @param hStream       Stream handle                                */
/* @param hStateCtxt    State context handle                         */
/* @param handle        Store handle of the state                    */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_deleteState(ismStore_StreamHandle_t hStream,
                                   void *hStateCtxt,
                                   ismStore_Handle_t handle)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s. hStream %u, Handle 0x%lx\n", __FUNCTION__, hStream, handle);

   rc = (*ismStore_global.pFnDeleteState)(hStream, hStateCtxt, handle);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

/*********************************************************************/
/* Store Dump                                                        */
/*                                                                   */
/* Dumps the content of the store.                                   */
/*                                                                   */
/* @param filename      File name of the output file                 */
/*                      If the name is STDOUT or STDERR then the     */
/*                      standard out/error stream is used.           */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_dump(char *filename)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "ism_store_dump(filename %s)\n", filename);

   pthread_mutex_lock(&ismStore_Mutex);

   rc = (ismStore_global.pFnDump ? (*ismStore_global.pFnDump)(filename) : ISMRC_StoreNotAvailable);

   pthread_mutex_unlock(&ismStore_Mutex);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

/*********************************************************************/
/* Store Dump with callbacks                                         */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_dumpCB(ismPSTOREDUMPCALLBACK callback, void *pContext)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s\n", __FUNCTION__);

   pthread_mutex_lock(&ismStore_Mutex);

   rc = (ismStore_global.pFnDumpCB ? (*ismStore_global.pFnDumpCB)(callback, pContext) : ISMRC_StoreNotAvailable);

   pthread_mutex_unlock(&ismStore_Mutex);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

/*********************************************************************/
/* Stop delivering async complete callbacks to the Engine            */
/*                                                                   */
/* Used by the Engine to instruct the Store to stop delivery of      */
/* async complete callbacks. Any pending callbacks are discarded.    */
/* If the Store cannot stop the async delivery threads it returns    */
/* ISMRC_StoreBusy, in which case the Engine should try again.       */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_stopCallbacks(void) 
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s\n", __FUNCTION__);

   pthread_mutex_lock(&ismStore_Mutex);

   /* if the store is not running return OK */
   rc = (ismStore_global.pFnStopCB ? (*ismStore_global.pFnStopCB)() : ISMRC_OK);

   pthread_mutex_unlock(&ismStore_Mutex);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

/*********************************************************************/
/* Get Stats about the Callbacks from  async commits completing      */
/*                                                                   */
/* Stats are of two types (see ismStore_AsyncThreadCBStats_t).       */
/* Some stats relate to a period (of ~5mins) e.g. how many CBs were  */
/* completed in this period. Others stats are "live" at the time     */
/* of the API call                                                   */
/*                                                                   */
/* @param pTotalReadyCBs     Callbacks that can be called ASAP       */
/* @param pTotalWaitingCBs   Callbacks waiting for commit to complete*/
/* @param pNumThreads        On input - array length that            */
/*                           pCBThreadStats points to.               */
/*                           On output if rc = StoreRC_BadParameter: */
/*                           then number of threads stats needed for */
/*                           succesful output                        */
/*                           On output if rc = OK: Number of thread  */
/*                           stats structure filled out with data    */
/* @param pCBThreadStats     Caller allocated/freed array of         */
/*                           ismStore_AsyncThreadCBStats_t that      */
/*                           this function will fill with data       */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_getAsyncCBStats(uint32_t *pTotalReadyCBs, uint32_t *pTotalWaitingCBs,
                                       uint32_t *pNumThreads,
                                       ismStore_AsyncThreadCBStats_t *pCBThreadStats)
{
    int32_t rc = ISMRC_OK;

    TRACE(9, "Entry: %s\n", __FUNCTION__);

    rc = (*ismStore_global.pFnGetAsyncCBStats)(pTotalReadyCBs, pTotalWaitingCBs,
                                               pNumThreads, pCBThreadStats);

    ismSTORE_SET_ERROR(rc)
    TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

    return rc;
}

/* ----------         Start of Recovery Mode APIs         ---------- */

/*********************************************************************/
/* Get next Generation ID                                            */
/*                                                                   */
/* A recovery function to iterate over the available generations.    */
/*                                                                   */
/*                                                                   */
/* @param pIterator     A pointer to an iterator handle.             */
/*                      It should be initialized to NULL on first    */
/*                      call and left unchanged for the rest of the  */
/*                      calls.                                       */
/* @param pGenId        Pointer to the generation id                 */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_getNextGenId(ismStore_IteratorHandle *pIterator,
                                    ismStore_GenId_t *pGenId)
{
   int32_t rc = ISMRC_OK;

   rc = (ismStore_global.pFnGetNextGenId ? (*ismStore_global.pFnGetNextGenId)(pIterator, pGenId) : ISMRC_StoreNotAvailable);

   ismSTORE_SET_ERROR(rc)

   return rc;
}

/*********************************************************************/
/* Get next Record                                                   */
/*                                                                   */
/* A recovery function to iterate over the available records of a    */
/* given type and a given generation. The first generation that is   */
/* returned by this function is the management generation.           */
/*                                                                   */
/* The pData field in the pRecord structure should point to a        */
/* pre-allocated buffer and the DataLength field should indicate the */
/* length of this buffer. The store copies the data into the buffer, */
/* and set the DataLength field to the number of bytes copied.       */
/* If the buffer is too small, the DataLength is set to the actual   */
/* data length, and an ISMRC_StoreBufferTooSmall code is returned.   */
/*                                                                   */
/* @param pIterator     A pointer to an iterator handle.             */
/*                      It should be initialized to NULL on first    */
/*                      call and left unchanged for the rest of the  */
/*                      calls with the same type and genId.          */
/* @param recordType    Record type                                  */
/* @param genId         Store generation identifier                  */
/* @param pHandle       Pointer to the returned Store record handle  */
/* @param pRecord       Pointer to the record structure              */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_getNextRecordForType(ismStore_IteratorHandle *pIterator,
                                            ismStore_RecordType_t recordType,
                                            ismStore_GenId_t genId,
                                            ismStore_Handle_t *pHandle,
                                            ismStore_Record_t *pRecord)
{
   int32_t rc = ISMRC_OK;

   if (recordType < ismSTORE_DATATYPE_MIN_EXTERNAL || recordType > ismSTORE_DATATYPE_MAX_EXTERNAL)
   {
      rc = ISMRC_ArgNotValid;
      ism_common_setErrorData(rc, "%s", "recordType");
   }
   else
   {
      rc = (ismStore_global.pFnGetNextRecordForType ?
                    (*ismStore_global.pFnGetNextRecordForType)(pIterator, recordType, genId, pHandle, pRecord) :
                    ISMRC_StoreNotAvailable);
   }
   ismSTORE_SET_ERROR(rc)

   return rc;
}

/*********************************************************************/
/* Read a Record                                                     */
/*                                                                   */
/* A recovery function to read the content of a record in the given  */
/* handle.                                                           */
/*                                                                   */
/* The pFrags field in the pRecord structure should point to a       */
/* pre-allocated array of buffers and the pFragsLengths field should */
/* indicate the length of each buffer in the array. The store copies */
/* the data into the buffers, and set the pFragsLength field to the  */
/* number of bytes copied to each fragment.                          */
/* If the buffers are too small, the DataLength is set to the actual */
/* data length, and an ISMRC_StoreBufferTooSmall code is returned.   */
/* If block is zero the function may return ISMRC_WouldBlock         */
/* to indicate that the record is currently not in memory (pRecord   */
/* is not modified in this case).                                    */
/*                                                                   */
/* @param handle        Handle of the required record                */
/* @param pRecord       Pointer to the record structure              */
/* @param block         If set the record is obtained even if it is  */
/*                      not currently available in memory            */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*         ISMRC_StoreBufferTooSmall indicates a larger buffer is    */
/*         needed to read the record                                 */
/*         ISMRC_WouldBlock indicates the record was not read   */
/*         since it is not in memory (and block is zero)             */
/*********************************************************************/
XAPI int32_t ism_store_readRecord(ismStore_Handle_t  handle,
                                  ismStore_Record_t *pRecord,
                                  uint8_t block)
{
   int32_t rc = ISMRC_OK;

   rc = (ismStore_global.pFnReadRecord ? (*ismStore_global.pFnReadRecord)(handle, pRecord, block) : ISMRC_StoreNotAvailable);

   ismSTORE_SET_ERROR(rc)

   return rc;
}

/*********************************************************************/
/* Get next References                                               */
/*                                                                   */
/* A recovery function to iterate over the available references of   */
/* a given owner and a given generation.                             */
/*                                                                   */
/*                                                                   */
/* @param pIterator     A pointer to an iterator handle.             */
/*                      It should be initialized to NULL on first    */
/*                      call and left unchanged for the rest of the  */
/*                      calls with the same owner and genId.         */
/* @param hOwnerHandle  Store handle of the references' owner        */
/* @param genId         Store generation identifier                  */
/* @param pHandle       Pointer to the returned Store record handle  */
/* @param pReference    Pointer to the reference structure           */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_getNextReferenceForOwner(ismStore_IteratorHandle *pIterator,
                                                ismStore_Handle_t hOwnerHandle,
                                                ismStore_GenId_t genId,
                                                ismStore_Handle_t    *pHandle,
                                                ismStore_Reference_t *pReference)
{
   int32_t rc = ISMRC_OK;

   rc = (ismStore_global.pFnGetNextReferenceForOwner ?
                 (*ismStore_global.pFnGetNextReferenceForOwner)(pIterator, hOwnerHandle, genId, pHandle, pReference) :
                 ISMRC_StoreNotAvailable);

   ismSTORE_SET_ERROR(rc)

   return rc;
}

/*********************************************************************/
/* Get next State                                                    */
/*                                                                   */
/* A recovery function to iterate over the available stats of        */
/* a given owner.                                                    */
/*                                                                   */
/*                                                                   */
/* @param pIterator     A pointer to an iterator handle.             */
/*                      It should be initialized to NULL on first    */
/*                      call and left unchanged for the rest of the  */
/*                      calls with the same owner.                   */
/* @param hOwnerHandle  Store handle of the references' owner        */
/* @param pHandle       Pointer to the returned Store record handle  */
/* @param pState        Pointer to the state structure               */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_getNextStateForOwner(ismStore_IteratorHandle *pIterator,
                                            ismStore_Handle_t hOwnerHandle,
                                            ismStore_Handle_t    *pHandle,
                                            ismStore_StateObject_t *pState)
{
   int32_t rc = ISMRC_OK;

   rc = (ismStore_global.pFnGetNextStateForOwner ?
                 (*ismStore_global.pFnGetNextStateForOwner)(pIterator, hOwnerHandle, pHandle, pState) :
                 ISMRC_StoreNotAvailable);

   ismSTORE_SET_ERROR(rc)

   return rc;
}

/*********************************************************************/
/* Recovery completed                                                */
/*                                                                   */
/* A recovery function to enable release of resources used during    */
/* the recovery process.                                             */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_recoveryCompleted(void)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s\n", __FUNCTION__);

   pthread_mutex_lock(&ismStore_Mutex);

   rc = (ismStore_global.pFnRecoveryCompleted ? (*ismStore_global.pFnRecoveryCompleted)() : ISMRC_StoreNotAvailable);

   pthread_mutex_unlock(&ismStore_Mutex);

   ismSTORE_SET_ERROR(rc)
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

/* ----------          End of Recovery Mode APIs          ---------- */


/*********************************************************************/
/*                                                                   */
/* End of store.c                                                    */
/*                                                                   */
/*********************************************************************/
