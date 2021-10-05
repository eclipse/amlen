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
/* Module Name: mccWildcardBFSet.h                                   */
/*                                                                   */
/* Description: A set of wildcard Bloom Filters for efficient lookup */
/*                                                                   */
/*********************************************************************/
#ifndef __WCBFSetI_DEFINED
#define __WCBFSetI_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include "mccLUSinternal.h"
#include "mccLookupSet.h"

typedef struct mcc_wcbfs_WCBFSet_t     * mcc_wcbfs_WCBFSetHandle_t;
typedef ismCluster_RemoteServerHandle_t  mcc_wcbfs_BFLookupHandle_t;


/*********************************************************************/
/* Create a new WCBFSet                                              */
/*                                                                   */
/* @param phWCBFSetHandle Pointer to a WCBFSet handle that will      */
/*                        point to the new WCBFSet on success.       */ 
/*                        Output parameter                           */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int mcc_wcbfs_createWCBFSet(mcc_wcbfs_WCBFSetHandle_t *phWCBFSetHandle);

/*********************************************************************/
/* Delete a WCBFSet and free all its resources                       */
/*                                                                   */
/* @param hWCBFSetHandle  Handle of the WCBFSet to delete            */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int mcc_wcbfs_deleteWCBFSet(mcc_wcbfs_WCBFSetHandle_t hWCBFSetHandle);

/*********************************************************************/
/* Add a new wildcard BF to the WCBFSet                              */
/*                                                                   */
/* @param hWCBFSetHandle  Handle of the WCBFSet                      */
/* @param pBFIndex      Pointer to a BF index that will point to the */
/*                      new BF index on success. Output parameter    */
/* @param pBFBytes      Pointer to an array of bytes representing    */
/*                      the BF                                       */
/* @param BFLen         Length (in bytes) of the BF                  */
/* @param hLookupHandle A lookup handle to associate with the BF     */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int mcc_wcbfs_addBF(mcc_wcbfs_WCBFSetHandle_t hWCBFSetHandle, int BFIndex, mcc_hash_t *pHashParams,
                       const char *pBFBytes, size_t BFLen, mcc_wcbfs_BFLookupHandle_t hLookupHandle);


/*********************************************************************/
/* Update a BF in the WCBFSet                                        */
/*                                                                   */
/* @param hWCBFSetHandle  Handle of the WCBFSet                      */
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
XAPI int mcc_wcbfs_updateBF(mcc_wcbfs_WCBFSetHandle_t hWCBFSetHandle, int BFIndex,
                          const int *pUpdates, int updatesLen);

/*********************************************************************/
/* Delete a BF from the WCBFSet                                      */
/*                                                                   */
/* @param hWCBFSetHandle  Handle of the WCBFSet                      */
/* @param BFIndex       Index of the BF to delete                    */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int mcc_wcbfs_deleteBF(mcc_wcbfs_WCBFSetHandle_t hWCBFSetHandle, int BFIndex);

/*********************************************************************/
/* Add a pattern to a wildcard BF in the WCBFSet                     */
/*                                                                   */
/* @param hWCBFSetHandle  Handle of the WCBFSet                      */
/* @param BFIndex       Index of the BF                              */
/* @param pPattern      Pointer to the pattern structure             */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int mcc_wcbfs_addPattern(mcc_wcbfs_WCBFSetHandle_t hWCBFSetHandle, int BFIndex,
                              mcc_lus_Pattern_t *pPattern);

/*********************************************************************/
/* Add a pattern to a wildcard BF in the WCBFSet                     */
/*                                                                   */
/* @param hWCBFSetHandle  Handle of the WCBFSet                      */
/* @param BFIndex       Index of the BF                              */
/* @param patternId     Pattern ID                                   */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int mcc_wcbfs_deletePattern(mcc_wcbfs_WCBFSetHandle_t hWCBFSetHandle, int BFIndex,
                                 uint64_t patternId);

/*********************************************************************/
/* Perform lookup in the WCBFSet                                     */
/*                                                                   */
/* @param hWCBFSetHandle  Handle of the WCBFSet                      */
/* @param pTopic        Pointer to the lookup topic                  */
/* @param topicLen      Length of the pTopic string                  */
/* @param phResults     An array of BF handles used as input/Output  */
/*                      parameter.                                   */
/*                      On input holds the list of handles that were */
/*                      already matched.                             */
/*                      On output hold the list of handles that      */
/*                      combines the input handles and the WCBFSet   */
/*                      lookup results.                              */
/* @param resultsLen    Length of the phResults array                */
/* @param pNumResults   On input a pointer to the number of matched  */
/*                      handles in the phResults array.              */
/*                      On output a pointer to the total number of   */
/*                      results returned in the phResults array.     */
/*                      Input/Output parameter                       */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int mcc_wcbfs_lookup(mcc_wcbfs_WCBFSetHandle_t hWCBFSetHandle, char *pTopic, int topicLen, uint8_t *skip,
                        mcc_wcbfs_BFLookupHandle_t *phResults, int resultsLen, int *pNumResults);



#ifdef __cplusplus
}
#endif

#endif /* __WCBFSetI_DEFINED */




