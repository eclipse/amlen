/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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

//*********************************************************************
/// @file exportCrypto.h
/// @brief API functions reading and writing encrypted files
///
/// Uses OpenSSL (libcrypto) to read and write encrypted data to files
//*********************************************************************

#ifndef EXPORTCRYPTO_H_DEFINED
#define EXPORTCRYPTO_H_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include "engineCommon.h"
#include "memHandler.h"

typedef struct tag_ieieEncryptedFile_t    *ieieEncryptedFileHandle_t;              // defined in exportCryptoInternal.h

typedef enum tag_ieieDataType_t {
    ieieDATATYPE_BYTES                      = 0,    ///< Generic, used for testing.
    ieieDATATYPE_EXPORTEDMESSAGERECORD      = 1,    ///< Message data
    ieieDATATYPE_EXPORTEDCLIENTSTATE        = 2,    ///< Client State information (ieieClientStateInfo_t)
    ieieDATATYPE_EXPORTEDRESOURCEFILEHEADER = 3,    ///< Header containing information about the exported resources
    ieieDATATYPE_EXPORTEDRESOURCEFILEFOOTER = 4,    ///< Footer containing information about the exported resources
    ieieDATATYPE_EXPORTEDSUBSCRIPTION       = 5,    ///< Non-shared (or client shared) subscription
    ieieDATATYPE_EXPORTEDGLOBALLYSHAREDSUB  = 6,    ///< Globally shared subscription
    ieieDATATYPE_DIAGCLIENTDUMPHEADER       = 7,    ///< Header for a dump of the client state table (data is JSON)
    ieieDATATYPE_DIAGCLIENTDUMPCHAIN        = 8,    ///< Client state table chain as part of a client state table dump(data is JSON)
    ieieDATATYPE_DIAGCLIENTDUMPFOOTER       = 9,    ///< Footer for a dump of the client state table (data is JSON)
    ieieDATATYPE_EXPORTEDQNODE_SIMPLE       = 10,   ///< SimpleQ        MessageReference
    ieieDATATYPE_EXPORTEDQNODE_INTER        = 11,   ///< IntermediateQ  MessageReference
    ieieDATATYPE_EXPORTEDQNODE_MULTI        = 12,   ///< MultiConsumerQ MessageReference
    ieieDATATYPE_EXPORTEDQNODE_MULTI_INPROG = 13,   ///< MultiConsumerQ Node in process of being delivered
    ieieDATATYPE_EXPORTEDRETAINEDMSG        = 14,   ///< Retained MessageReference
    ieieDATATYPE_TRACEDUMPHEADER            = 15,   ///< Header for diagnostic dump of in memory trace
    ieieDATATYPE_TRACEDUMPFOOTER            = 16,   ///< Footer for diagnostic dump of in memory trace
    ieieDATATYPE_TRACEDUMPTHREADHEADER      = 17,   ///< Header for Trace from one thread
    ieieDATATYPE_TRACEDUMPTHREADIDENTS      = 18,   ///< List of trace points for one thread
    ieieDATATYPE_TRACEDUMPTHREADVALUES      = 19,   ///< List of values saved at trace points for one thread
    ieieDATATYPE_DIAGSUBDETAILSHEADER       = 20,   ///< Header for diagnostic output of subscription details
    ieieDATATYPE_DIAGSUBDETAILSUBSCRIPTION  = 21,   ///< Details about a particular subscription
    ieieDATATYPE_DIAGSUBDETAILSFOOTER       = 22,   ///< Footer for diagnostic output of subscription details
    ieieDATATYPE_LAST,                              ///< Marker for end of enumeration - add new values before this
} ieieDataType_t;


typedef struct tag_ieieFragmentedExportData_t
{
   uint32_t              FragsCount; /* Number of fragments from which a record*/
                                     /* is to be assembled. The store          */
                                     /* constructs the record by reassembling  */
                                     /* FragsCount fragments from the pFrags   */
                                     /* array into a single buffer.            */
   void                **pFrags;     /* Record fragments array. A pointer to   */
                                     /* an array of buffers comprising the     */
                                     /* fragments of the record. Only the first*/
                                     /* FragsCount buffers are used. Fragments */
                                     /* are reassembled according to the order */
                                     /* they appear in the pFrags array.       */
   uint32_t               *pFragsLengths; /* Lengths of record fragments in      */
                                     /* pFrags. A pointer to an array of       */
                                     /* integer each holding the length of the */
                                     /* fragment in the corresponding index in */
                                     /* the pFrags array. Each frags size (plus*/
                                     /* blocksize) must be <= signed INT_MAX   */
} ieieFragmentedExportData_t;


ieieEncryptedFileHandle_t ieie_createEncryptedFile( ieutThreadData_t *pThreadData
                                                   , iemem_memoryType memType
                                                   , const char *filePath
                                                   , const char *password);

int32_t ieie_finishWritingEncryptedFile ( ieutThreadData_t *pThreadData
                                        , ieieEncryptedFileHandle_t file );

//
int32_t ieie_finishReadingEncryptedFile ( ieutThreadData_t *pThreadData
                                        , ieieEncryptedFileHandle_t file );

int32_t ieie_exportData( ieutThreadData_t *pThreadData
                       , ieieEncryptedFileHandle_t file
                       , ieieDataType_t dataType
                       , uint64_t dataId
                       , int dataLen
                       , void *data);

int32_t ieie_exportDataFrags( ieutThreadData_t *pThreadData
                            , ieieEncryptedFileHandle_t file
                            , ieieDataType_t dataType
                            , uint64_t dataId
                            , ieieFragmentedExportData_t *pFragsData);


//opens and encrypted file for reading!
ieieEncryptedFileHandle_t ieie_OpenEncryptedFile( ieutThreadData_t *pThreadData
                                                , iemem_memoryType memType
                                                , const char *filePath
                                                , const char *password);


int32_t ieie_importData( ieutThreadData_t *pThreadData
                       , ieieEncryptedFileHandle_t file
                       , ieieDataType_t *pDataType
                       , uint64_t *pDataId
                       , size_t *dataLen
                       , void **data);

void ieie_freeReadExportedData( ieutThreadData_t *pThreadData
                              , iemem_memoryType memType
                              , void *data);

#endif

