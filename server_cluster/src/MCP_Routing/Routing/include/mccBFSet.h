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

/*********************************************************************/
/*                                                                   */
/* Module Name: mccBFSet.h                                           */
/*                                                                   */
/* Description: Bloom Filter Set for efficient lookup of multiple BFs*/
/*                                                                   */
/*********************************************************************/
#ifndef __BFSETI_DEFINED
#define __BFSETI_DEFINED

#include "hashFunction.h"
#include "mccLUSinternal.h"
#include "mccLookupSet.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mcc_bfs_BFSet_t          * mcc_bfs_BFSetHandle_t;
typedef ismCluster_RemoteServerHandle_t   mcc_bfs_BFLookupHandle_t;

/* A structure holding BF Set parameters.                                      */
typedef struct
{
   int               numBFs;         /* Number of BFs that can be held in the  */
                                     /* BFSet                                  */
   int               maxBFLen;       /* Maximal Length (in bytes) of a BF in   */
                                     /* the BFSet                              */
   int               numHashValues;  /* Number of hash values to use in lookup */
   mcc_hash_HashType_t hashType;     /* Hash function used by this BFSet       */
   int                id;
   size_t            *dbg_cnt;   
} mcc_bfs_BFSetParameters_t;

/*********************************************************************/
/* Create a new BFSet                                                */
/*                                                                   */
/* @param phBFSetHandle Pointer to a BFSet handle that will point to */
/*                      the new BFSet on success. Output parameter   */
/* @param pBFSetParams  Pointer to the BFSet parameters              */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int mcc_bfs_createBFSet(mcc_bfs_BFSetHandle_t *phBFSetHandle, 
                             mcc_bfs_BFSetParameters_t *pBFSetParams);

/*********************************************************************/
/* Delete a BFSet and free all its resources                         */
/*                                                                   */
/* @param hBFSetHandle  Handle of the BFSet to delete                */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int mcc_bfs_deleteBFSet(mcc_bfs_BFSetHandle_t hBFSetHandle);

/*********************************************************************/
/* Resize a BFSet                                                    */
/* The existing BFs in the BFSet are resized while maintaining their */
/* properties.                                                       */
/* It is allowed to increase the BFSet size (numBFs) and/or the size */
/* of the BFs (BFLen).                                               */
/* It is not allowed to decrease numBFs or BFLen.                    */
/*                                                                   */
/* @param hBFSetHandle  Handle of the BFSet to resize                */
/* @param pBFSetParams  Pointer to the new BFSet parameters          */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
//XAPI int mcc_bfs_resizeBFSet(mcc_bfs_BFSetHandle_t hBFSetHandle, 
//                             mcc_bfs_BFSetParameters_t *pBFSetParams);

/*********************************************************************/
/* Add a new BF to the BFSet                                         */
/*                                                                   */
/* @param hBFSetHandle  Handle of the BFSet                          */
/* @param pBFIndex      Pointer to a BF index that will point to the */
/*                      new BF index on success. Output parameter    */
/* @param pBFBytes      Pointer to an array of bytes representing    */
/*                      the BF                                       */
/* @param BFLen         Length (in bytes) of the BF. The length      */
/*                      could be smaller than the BFSet's max BFLen  */
/* @param hLookupHandle A lookup handle to associate with the BF     */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int mcc_bfs_addBF(mcc_bfs_BFSetHandle_t hBFSetHandle, int pBFIndex,
                       const char *pBFBytes, int BFLen, mcc_bfs_BFLookupHandle_t hLookupHandle);

/*********************************************************************/
/* Replace the entire content of a BF in the BFSet                   */
/* The index and the lookup handle of the BF are not changed         */
/*                                                                   */
/* @param hBFSetHandle  Handle of the BFSet                          */
/* @param BFIndex       Index of the BF to replace                   */
/* @param pBFBytes      Pointer to an array of bytes representing    */
/*                      the BF                                       */
/* @param BFLen         Length (in bytes) of the BF. The length      */
/*                      could be smaller than the BFSet's max BFLen  */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int mcc_bfs_replaceBF(mcc_bfs_BFSetHandle_t hBFSetHandle, int BFIndex,
                           const char *pBFBytes, int BFLen);

/*********************************************************************/
/* Update a BF in the BFSet                                          */
/*                                                                   */
/* @param hBFSetHandle  Handle of the BFSet                          */
/* @param BFIndex       Index of the BF to update                    */
/* @param pUpdates      Pointer to an array of integers where each   */
/*                      integer represents a position of a single    */
/*                      bit in the BF to set or reset.               */
/*                      Updates are performed as follows             */
/*                      if position > 0 set the bit at position-1    */
/*                      if position < 0 reset the bit at |position|-1*/
/* @param updatesLen    Length (in bytes) of the pUpdates array      */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int mcc_bfs_updateBF(mcc_bfs_BFSetHandle_t hBFSetHandle, int BFIndex,
                          const int *pUpdates, int updatesLen);

/*********************************************************************/
/* Delete a BF from the BFSet                                        */
/*                                                                   */
/* @param hBFSetHandle  Handle of the BFSet                          */
/* @param BFIndex       Index of the BF to delete                    */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int mcc_bfs_deleteBF(mcc_bfs_BFSetHandle_t hBFSetHandle, int BFIndex);

/*********************************************************************/
/* Perform lookup in the BFSet                                       */
/*                                                                   */
/* @param hBFSetHandle  Handle of the BFSet                          */
/* @param pTopic        Pointer to the lookup topic                  */
/* @param topicLen      Length of the pTopic string                  */
/* @param phResults     An array of BF handles to hold the lookup    */
/*                      results. Output parameter                    */
/* @param BFInds        An array of BF indeces to include in the     */
/*                      returned list                                */
/* @param BFIndsLen     Length of the BFInds array. 0 => all BFs     */
/* @param resultsLen    Length of the phResults array                */
/* @param pNumResults   Pointer to the number of results returned in */
/*                      the phResults array. Output parameter        */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
//XAPI int mcc_bfs_lookup(mcc_bfs_BFSetHandle_t hBFSetHandle, int *pPositions, int positionsLen,
XAPI int mcc_bfs_lookup(mcc_bfs_BFSetHandle_t hBFSetHandle, char *pTopic, size_t topicLen, uint8_t *skip,
                        mcc_bfs_BFLookupHandle_t *phResults, int resultsLen, int *pNumResults);




#ifdef __cplusplus
}
#endif

#endif /* __BFSETI_DEFINED */




