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
/* Module Name: mccLookupSet.h                                       */
/*                                                                   */
/* Description: Remote Subscription Manager Lookup Set API           */
/*                                                                   */
/*********************************************************************/
#ifndef __LUSETI_DEFINED
#define __LUSETI_DEFINED

#include "cluster.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mcc_lus_LUSet_t      * mcc_lus_LUSetHandle_t;

/* A structure representing a wildcard BF pattern.                             */
typedef struct
{
  uint64_t           patternId;      /* Pattern identifier                     */
  uint16_t           numPluses;      /* Number of pluses in pPlusLevels        */
  const uint16_t    *pPlusLevels;    /* Array holding the plus levels          */
  uint16_t           hashLevel;      /* Level of hash (0 if none)              */
  uint16_t           patternLen;     /* Number of level in the pattern         */
} mcc_lus_Pattern_t;

/*********************************************************************/
/* Create a new LUSet                                                */
/*                                                                   */
/* @param phLUSetHandle Pointer to a LUSet handle that will point to */
/*                      the new LUSet on success. Output parameter   */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI ism_rc_t mcc_lus_createLUSet(mcc_lus_LUSetHandle_t *phLUSetHandle);

/*********************************************************************/
/* Delete a LUSet and free all its resources                         */
/*                                                                   */
/* @param phLUSetHandle Pointer to the handle of the LUSet to delete.*/
/*                      After the LUSet is deleted the handle is set */
/*                      to NULL. Input/output parameter              */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI ism_rc_t mcc_lus_deleteLUSet(mcc_lus_LUSetHandle_t *phLUSetHandle);

/*********************************************************************/
/* Add a new BF to the LUSet or replace an existing BF with a new BF */
/*                                                                   */
/* @param hLUSetHandle  Handle of the LUSet                          */
/* @param hRemoteServer Pointer to the remote server handle.         */
/*                      The handle includes an index that idintifies */
/*                      the remote server and the Engine handle      */
/*                      associated with the remote server            */
/* @param pBFBytes      Pointer to an array of bytes representing    */
/*                      the BF                                       */
/* @param BFLen         Length (in bytes) of the BF. The length      */
/*                      could be smaller than the LUSet's max BFLen  */
/* @param hashType      Hash function to use for this BF             */
/* @param numHashValues Number of hash values to use for this BF     */
/* @param fIsWildcard   Indicates if this is a wildcard BF or an     */
/*                      exact BF. Each remote server may have one    */
/*                      exact BF and one wildcard BF                 */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI ism_rc_t mcc_lus_addBF(mcc_lus_LUSetHandle_t hLUSetHandle,
                       ismCluster_RemoteServerHandle_t hRemoteServer,
                       const char *pBFBytes, size_t BFLen, mcc_hash_HashType_t hashType,
                       int numHashValues, int fIsWildcard);

/*********************************************************************/
/* Update a BF in the LUSet                                          */
/*                                                                   */
/* @param hLUSetHandle  Handle of the LUSet                          */
/* @param hRemoteServer Pointer to the remote server handle.         */
/* @param fIsWildcard   Indicates if this is a wildcard or exact BF  */
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
XAPI ism_rc_t mcc_lus_updateBF(mcc_lus_LUSetHandle_t hLUSetHandle,
                          ismCluster_RemoteServerHandle_t hRemoteServer,
                          int fIsWildcard, const int32_t *pUpdates, size_t updatesLen);

/*********************************************************************/
/* Delete a BF from the LUSet                                        */
/*                                                                   */
/* @param hLUSetHandle  Handle of the LUSet                          */
/* @param hRemoteServer Pointer to the remote server handle.         */
/* @param fIsWildcard   Indicates if this is a wildcard or exact BF  */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI ism_rc_t mcc_lus_deleteBF(mcc_lus_LUSetHandle_t hLUSetHandle,
                          ismCluster_RemoteServerHandle_t hRemoteServer,
                          int fIsWildcard);

/*********************************************************************/
/* Add a pattern to a wildcard BF in the LUSet                       */
/*                                                                   */
/* @param hWCLUSetHandle  Handle of the WCLUSet                      */
/* @param hRemoteServer   Pointer to the remote server handle.       */
/* @param pPattern        Pointer to the pattern structure           */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI ism_rc_t mcc_lus_addPattern(mcc_lus_LUSetHandle_t hLUSetHandle,
                            ismCluster_RemoteServerHandle_t hRemoteServer,
                            mcc_lus_Pattern_t *pPattern);

/*********************************************************************/
/* Delete a pattern from a wildcard BF in the LUSet                  */
/*                                                                   */
/* @param hWCLUSetHandle  Handle of the WCLUSet                      */
/* @param hRemoteServer   Pointer to the remote server handle.       */
/* @param patternId       Pattern ID                                 */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI ism_rc_t mcc_lus_deletePattern(mcc_lus_LUSetHandle_t hLUSetHandle,
                               ismCluster_RemoteServerHandle_t hRemoteServer,
                               uint64_t patternId);

/*********************************************************************/
/* Delete a server from the LUSet                                    */
/*                                                                   */
/* @param hWCLUSetHandle  Handle of the WCLUSet                      */
/* @param hRemoteServer   Pointer to the remote server handle.       */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI ism_rc_t mcc_lus_deleteServer(mcc_lus_LUSetHandle_t hLUSetHandle,
                               ismCluster_RemoteServerHandle_t hRemoteServer);

/*********************************************************************/
/* Set route-all value for a remote server                           */
/*                                                                   */
/* @param hLUSetHandle  Handle of the LUSet                          */
/* @param hRemoteServer Pointer to the remote server handle.         */
/* @param fRouteAll     The route-all value for the remote server    */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI ism_rc_t mcc_lus_setRouteAll(mcc_lus_LUSetHandle_t hLUSetHandle,
                          ismCluster_RemoteServerHandle_t hRemoteServer,
                          int fRouteAll);

/*********************************************************************/
/* Perform lookup in the LUSet                                       */
/*                                                                   */
/* @param hLUSetHandle  Handle of the LUSet                          */
/* @param pLookupInfo   Pointer to the Cluster lookup structure      */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI ism_rc_t mcc_lus_lookup(mcc_lus_LUSetHandle_t hLUSetHandle,
                        ismCluster_LookupInfo_t *pLookupInfo);


#ifdef __cplusplus
}
#endif

#endif /* __LUSETI_DEFINED */




