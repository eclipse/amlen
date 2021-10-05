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
/* Module Name: storeMemory.h                                        */
/*                                                                   */
/* Description: Store memory component header file                   */
/*                                                                   */
/*********************************************************************/
#ifndef __ISM_STORE_MEMORY_DEFINED
#define __ISM_STORE_MEMORY_DEFINED

/*********************************************************************/
/*                                                                   */
/* INCLUDES                                                          */
/*                                                                   */
/*********************************************************************/
#include <pthread.h>          /* pthread header file                 */
#include <stdlib.h>           /* C standard library header file      */
#include <string.h>           /* String and memory header file       */
#include <stdint.h>           /* Standard integer defns header file  */
#include <stdbool.h>          /* Standard boolean defns header file  */
#include <limits.h>           /* System limits header file           */

#include <store.h>            /* Store external header file          */
#include <config.h>
#include "storeInternal.h"
#include "storeHighAvailability.h"
#include "storeMemoryHA.h"
#include "storeDiskUtils.h"

#define USE_REFSTATS  1


/* Indicates whether an admin configuration registration is required or not */
#define ismSTORE_ADMIN_CONFIG  0

/*
 * Storage layout
 *
 * The storage is allocated in NVRAM/Shared-Memory such that it can survive
 * across restarts of the ISM server. The storage consists of two main regions:
 * management and data. The management region is always stored in the
 * storage memory. The data consists of multiple sub-regions, called generations,
 * such that at any point in the execution only one generation is active. New
 * data records are added into the active generation. Once the active generation
 * becomes full, the storage selects a new generation to act as the active
 * generation and evacuates the data of the previous generation to the disk.
 *
 */

// If we're in ADR mode, this forces the cache to be flushed, thus making the data persistent
#define ADR_WRITE_BACK(addr,len) \
   if (ismStore_global.CacheFlushMode == ismSTORE_CACHEFLUSH_ADR) \
       ism_store_memForceWriteBack((addr),(len));

/*********************************************************************/
/* Descriptor                                                        */
/*                                                                   */
/* Descriptor for each item in the Store. Lies at the start of each  */
/* granule.                                                          */
/*********************************************************************/
typedef struct ismStore_memDescriptor_t
{
   uint32_t                        TotalLength;           /* Total number of data bytes, including  */
                                                          /* linked granules.                       */
   uint32_t                        GranuleIndex;
   uint64_t                        Attribute;
   uint64_t                        State;
   ismStore_Handle_t               NextHandle;
   uint32_t                        DataLength;            /* Number of data bytes in this granule   */
   uint16_t                        DataType;
   uint8_t                         PoolId;
   uint8_t                         Pad[1];
} ismStore_memDescriptor_t;

/*********************************************************************/
/* Store Granule Pool                                                */
/*                                                                   */
/* Granule Pool in the Store.                                        */
/*********************************************************************/
typedef struct ismStore_memGranulePool_t
{
   uint64_t                        Offset;
   ismStore_Handle_t               hHead;
   ismStore_Handle_t               hTail;
   uint32_t                        GranuleCount;
   uint32_t                        GranuleSizeBytes;
   uint32_t                        GranuleDataSizeBytes;
   uint64_t                        MaxMemSizeBytes;
   uint8_t                         Reserved[32];
} ismStore_memGranulePool_t;

/*********************************************************************/
/* Store Generation Header                                           */
/*                                                                   */
/* Generation header item in the Store.                              */
/* This item lies at the start of the granule with no descriptor.    */
/*********************************************************************/
typedef struct ismStore_memGenToken_t
{
   ismStore_HANodeID_t             NodeId;
   uint64_t                        Timestamp;
} ismStore_memGenToken_t;

typedef struct ismStore_memGenHeader_t
{
   uint32_t                        StrucId;               /* 0xABCDAAAA */
   ismStore_GenId_t                GenId;
   uint8_t                         State;
   uint8_t                         PoolsCount;
   uint32_t                        DescriptorStructSize;  /* Used for backward compatibility                        */
   uint32_t                        SplitItemStructSize;   /* Used for backward compatibility                        */
   ismStore_memGenToken_t          GenToken;              /* Used for High-Availability                             */
   uint64_t                        Version;               /* Version Id                                             */
   uint64_t                        MemSizeBytes;          /* Memory size in bytes of the generation segment         */
   uint64_t                        RsrvPoolMemSizeBytes;  /* The reserved pool is NOT pre-allocated on startup.     */
                                                          /* Once one of the granule pools becomes empty, this      */
                                                          /* memory is added to the empty pool.                     */
   ismStore_memGranulePool_t       GranulePool[ismSTORE_GRANULE_POOLS_COUNT];
   uint64_t                        CompactSizeBytes;      /* The size of the compacted data in bytes. If the data   */
                                                          /* is not compacted the value is 0.                       */
   uint64_t                        StdDevBytes;           /* Standard Deviation of records in bytes                 */
   uint8_t                         Reserved[48];
} ismStore_memGenHeader_t;

#define ismSTORE_MEM_GENHEADER_STRUCID  0xABCDAAAA

#define ismSTORE_GEN_STATE_FREE             0
#define ismSTORE_GEN_STATE_ACTIVE           1
#define ismSTORE_GEN_STATE_CLOSE_PENDING    2
#define ismSTORE_GEN_STATE_WRITE_PENDING    3
#define ismSTORE_GEN_STATE_WRITE_COMPLETED  4

/*********************************************************************/
/* Store Management Header                                           */
/*                                                                   */
/* Management header item in the Store.                              */
/* This item lies at the start of the granule with no descriptor.    */
/*********************************************************************/
typedef struct ismStore_memMgmtHeader_t
{
   /*--- The first fields in this structure MUST be the same as   ---*/
   /*--- the fields of ismStore_memGenHeader_t                    ---*/
   uint32_t                        StrucId;               /* 0xABCDAAAB */
   ismStore_GenId_t                GenId;
   uint8_t                         State;
   uint8_t                         PoolsCount;
   uint32_t                        DescriptorStructSize;  /* Used for backward compatibility                        */
   uint32_t                        SplitItemStructSize;   /* Used for backward compatibility                        */
   ismStore_memGenToken_t          GenToken;              /* Used for High-Availability                             */
   uint64_t                        Version;
   uint64_t                        MemSizeBytes;          /* Memory size in bytes of the management segment         */
   uint64_t                        RsrvPoolMemSizeBytes;  /* The reserved pool is NOT pre-allocated on startup.     */
                                                          /* Once one of the granule pools becomes empty, this      */
                                                          /* memory is added to the empty pool.                     */
   ismStore_memGranulePool_t       GranulePool[ismSTORE_GRANULE_POOLS_COUNT];
   uint64_t                        CompactSizeBytes;      /* The size of the compacted data in bytes. If the data   */
                                                          /* is not compacted the value is 0.                       */
   uint64_t                        StdDevBytes;           /* Standard Deviation of records in bytes                 */
   uint8_t                         Reserved[48];
   /*---------------- END OF ismStore_memGenHeader_t ----------------*/

   uint64_t                        TotalMemSizeBytes;     /* Total memory size of the storage                       */
   uint8_t                         InMemGensCount;
   uint8_t                         ActiveGenIndex;
   uint8_t                         Pad[2];
   ismStore_GenId_t                ActiveGenId;
   ismStore_GenId_t                NextAvailableGenId;
   ismStore_Handle_t               InMemGenOffset[ismSTORE_MAX_INMEM_GENS];
   ismStore_Handle_t               GenIdHandle;
   uint8_t                         PersistToken[8];
   uint8_t                         Reserved2[108];

   /*-- The fields after the Reserved2 field are not sent with    --*/
   /*-- the MgmtHeader to the Standby node during runtime.        --*/
   ismStore_HASessionID_t          SessionId;             /* Identifies the particular instance of the
                                                           * High-Availability session.                             */
   uint16_t                        SessionCount;          /* Identifies the Primary and Standby nodes of the same
                                                           * SessionId. SessionCount is an even number on Primary
                                                           * nodes and odd number on Standby nodes.                 */
   uint8_t                         Role;                  /* Last role: PRIMARY, STANDBY, UNSYNC                    */
   uint8_t                         HaveData;
   uint8_t                         WasPrimary;
   uint8_t                         Pad2[5];
   uint64_t                        PrimaryTime;

} ismStore_memMgmtHeader_t;

#define ismSTORE_MEM_MGMTHEADER_STRUCID  0xABCDAAAB

#define ismSTORE_ROLE_PRIMARY               0
#define ismSTORE_ROLE_STANDBY               1
#define ismSTORE_ROLE_UNSYNC                2

/*********************************************************************/
/* Generation Id Chunk                                               */
/*                                                                   */
/* A chunk of generations identifiers used by the store.             */
/* Lies just after the descriptor.                                   */
/*********************************************************************/
typedef struct ismStore_memGenIdChunk_t
{
   uint16_t                        GenIdCount;
   ismStore_GenId_t                GenIds[1];
} ismStore_memGenIdChunk_t;

/*********************************************************************/
/* SplitItem                                                         */
/*                                                                   */
/* This item is a complex item. It lies just after the descriptor.   */
/* The data of this item is split across the store in the following  */
/* manner:                                                           */
/* 1. The header is stored in the management area pool #0            */
/* 2. The data is stored along with the header (if possible) or in   */
/*    the generation area. The hLargeData contains the handle of     */
/*    of the data itself. If the data is stored in this item, the    */
/*    hLargeData contains the creation generation id with offset 0.  */
/* 3. The reference chunks are stored in the generation area.        */
/*                                                                   */
/* The following record types are stored as SplitItem:               */
/* ISM_STORE_RECTYPE_QUEUE, ISM_STORE_RECTYPE_TOPIC,                 */
/* ISM_STORE_RECTYPE_SUBSC, ISM_STORE_RECTYPE_TRANS,                 */
/* ISM_STORE_RECTYPE_CLIENT, ISM_STORE_RECTYPE_BMGR,                 */
/* ISM_STORE_RECTYPE_REMSRV.                                         */
/*********************************************************************/
typedef struct ismStore_memSplitItem_t
{
   uint32_t                        Version;            /* Version number. Used to associate chunks  */
                                                       /* with this owner.                          */
   uint8_t                         Pad[4];
   ismStore_Handle_t               hLargeData;         /* Handle of the large data (if exists)      */
                                                       /* For small data length, the data is stored */
                                                       /* in this item itself, just after this      */
                                                       /* structure. In such a case, the hLargeData */
                                                       /* contains the creation generation is with  */
                                                       /* offset zero.                              */
   uint64_t                        DataLength;         /* Total data length                         */
   uint64_t                        MinActiveOrderId;   /* Minimum active reference order Id         */
   uint64_t                        pRefCtxt;           /* Pointer to the ReferenceContext which is  */
                                                       /* allocated by the store in the main memory.*/
                                                       /* This value is set to 0 during recovery    */
   uint64_t                        pStateCtxt;         /* Pointer to the StateContext which is      */
                                                       /* allocated by the store in the main memory.*/
                                                       /* This value is set to 0 during recovery    */
} ismStore_memSplitItem_t;

/*********************************************************************/
/* Reference                                                         */
/*                                                                   */
/* A reference to a record, with an associated status. References    */
/* are stored in order.                                              */
/*********************************************************************/
typedef struct ismStore_memReference_t
{
   ismStore_Handle_t               RefHandle;
   uint32_t                        Value;
   uint8_t                         State;
   uint8_t                         Flag;  // internal use only !!
   uint8_t                         Pad[2];
} ismStore_memReference_t;

/*********************************************************************/
/* Reference Chunk                                                   */
/*                                                                   */
/* A chunk of references which fits in a single granule.             */
/* Lies just after the descriptor.                                   */
/*********************************************************************/
typedef struct ismStore_memReferenceChunk_t
{
   ismStore_Handle_t               OwnerHandle;        /* Refers back to the owner record           */
   uint64_t                        BaseOrderId;
   uint32_t                        OwnerVersion;       /* Version of the owner record. Should be    */
                                                       /* the same as the version of the owner. If  */
                                                       /* it is different then the chunk is no      */
                                                       /* longer valid and should be discarded.     */
   uint32_t                        ReferenceCount;
   ismStore_memReference_t         References[1];
} ismStore_memReferenceChunk_t;

/*********************************************************************/
/* Reference State Chunk                                             */
/*                                                                   */
/* A chunk of reference state entries. Lies just after the           */
/* descriptor.                                                       */
/* The chunk contains the state of references that were updated by   */
/* the engine, and are stored on the disk.                           */
/*                                                                   */
/* The value of the RefState entry should be interpreted as follows: */
/* ismSTORE_REFSTATE_DELETED   - indicates that the reference at     */
/*                               index i was deleted by the engine,  */
/*                               so it should not be retrieved       */
/*                               during store recovery.              */
/* ismSTORE_REFSTATE_NOT_VALID - indicates that the reference at     */
/*                              index i was not changed by the       */
/*                              engine, so the state on the disk is  */
/*                              valid.                               */
/* Otherwise, the value in the RefState entry should be retrieved as */
/* the state of that reference.                                      */
/*********************************************************************/
typedef struct ismStore_memRefStateChunk_t
{
   uint32_t                        Rsrv4Version;       /* Reserved for owner version in management  */
                                                       /* small granule                             */
   uint32_t                        OwnerVersion;       /* Version of the owner record. Should be    */
                                                       /* the same as the version of the owner. If  */
                                                       /* it is different then the chunk is no      */
                                                       /* longer valid and should be discarded.     */
   ismStore_Handle_t               OwnerHandle;        /* Refers back to the owner record           */
   uint64_t                        BaseOrderId;        /* Base order Id of the chunk                */
   uint32_t                        RefStateCount;
   uint8_t                         RefStates[1];
} ismStore_memRefStateChunk_t;

#define ismSTORE_REFSTATE_DELETED    0xfe
#define ismSTORE_REFSTATE_NOT_VALID  0xff

/*********************************************************************/
/* Reference Context                                                 */
/*                                                                   */
/* Used to track references for a specific owner handle. Stored in   */
/* the main memory (not in the store).                               */
/*********************************************************************/

#if USE_REFSTATS
typedef struct
{
  uint32_t nCL; // Total Calls
  uint32_t nBH; // Before Head
  uint32_t nAT; // After Tail
  uint32_t nTL; // At Tail
  uint32_t nAC; // At Cache
  uint32_t nAF; // At Finger
  uint32_t nUF; // Use Finger
  uint32_t nBT; // Before Tail ; 
  uint32_t nLL; // LinkList Search
  uint32_t nEL; // LinkList Elements
  uint32_t b[8];// LinkList Elements
} ismStore_memRefStats_t ; 
#endif

typedef struct ismStore_refGenCache_t
{
   uint32_t                        cacheSize;
   uint32_t                        nextTrimSecs;
   ismStore_Handle_t               hRefCache[1] ; 
}  ismStore_refGenCache_t ; 

typedef struct ismStore_memRefGen_t
{
   uint64_t                        LowestOrderId;      /* Lowest order Id in the generation         */
   uint64_t                        HighestOrderId;     /* Highest order Id in the generation        */
   ismStore_Handle_t               hReferenceHead;     /* Handle of the 1st chunk in the generation.*/
                                                       /* The handle contains the generation Id     */
   ismStore_Handle_t               hReferenceTail;     /* Handle of the last chunk in the generation*/
   ismStore_refGenCache_t         *pRefCache;          /* Handle of the chunk before last           */
   uint32_t                        numChunks;
   uint32_t                        numRefs;
   struct ismStore_memRefGen_t    *Next;               /* Pointer to the next RefGen element        */
} ismStore_memRefGen_t;

typedef struct ismStore_memRefGenPool_t
{
   uint32_t                        Size;               /* Number of allocated elements              */
   int32_t                         Count;              /* Number of available elements              */
   uint16_t                        Index;              /* Pool Index                                */
   uint8_t                         fPendingTask;       /* Indicates a pending store thread task     */
   ismStore_memRefGen_t           *pHead;              /* Pointer to the first available element    */
  #if USE_REFSTATS
   ismStore_memRefStats_t          RefStats[2] ; 
  #endif
} ismStore_memRefGenPool_t;

#define ismSTORE_CHUNK_GAP_LWM     64
#define ismSTORE_CHUNK_GAP_HWM    256
typedef struct ismStore_memRefStateFingers_t
{
   uint64_t                       *BaseOid; 
   ismStore_Handle_t              *Handles;
   uint32_t                        ChunkGap;
   uint32_t                        NumAtEnd;
   uint32_t                        NumInArray;
   uint32_t                        NumInUse;
} ismStore_memRefStateFingers_t;

typedef struct ismStore_memReferenceContext_t
{
   ismSTORE_MUTEX_t                      *pMutex;
   ismStore_Handle_t                      OwnerHandle;        /* Handle of the owner in the store          */
   uint64_t                               HighestOrderId;     /* Highest reference order Id                */
   uint64_t                               NextPruneOrderId;   /* Next OrderId for prune references         */
   ismStore_memRefGenPool_t              *pRefGenPool;        /* Pointer to the RefGenPool which is used   */
                                                              /* for RefGen allocation.                    */
   ismStore_memRefGen_t                  *pRefGenHead;        /* Pointer to a linked-list of RefGen items, */
                                                              /* one for each generation. The list is      */
                                                              /* ordered from the oldest generation to the */
                                                              /* newest generation.                        */
   ismStore_memRefGen_t                  *pRefGenLast;        /* Used for recovery acceleration            */
   ismStore_memRefGen_t                  *pInMemRefGen[ismSTORE_MAX_INMEM_GENS];
                                                              /* Shortcuts to a RefGen item in the list,   */
                                                              /* for the in-memory store generations, by   */
                                                              /* the in-memory generation index.           */
   ismStore_memRefGen_t                  *pRefGenState;       /* Pointer to a RefGen item of the RefState  */
   ismStore_Handle_t                      hRFCacheChunk[2];   /* Handle of the last accessed RefState chunk*/
                                                              /* (Used as a small cache for performance)   */
   ismStore_memRefStateFingers_t         *pRFFingers;
   uint32_t                               RFChunksInUse;      /* Number of RefState chunks in use          */
                                                              /* (Used for the management generation only) */
   uint32_t                               RFChunksInUseHWM;   /* Dynamic high water mark for chunks in use */
   uint32_t                               RFChunksInUseLWM;   /* Dynamic low water mark for chunks in use  */
   uint32_t                               OwnerVersion;       /* Version of the owner                      */
} ismStore_memReferenceContext_t;

/*********************************************************************/
/* State                                                             */
/*                                                                   */
/* A State of a record. States are stored in order.                  */
/*********************************************************************/
typedef struct ismStore_memState_t
{
   uint32_t                        Value;
   uint8_t                         Flag;
   uint8_t                         Pad[3];
} ismStore_memState_t;

#define ismSTORE_STATEFLAG_EMPTY     0x0
#define ismSTORE_STATEFLAG_RESERVED  0x1
#define ismSTORE_STATEFLAG_VALID     0x2

/*********************************************************************/
/* State Chunk                                                       */
/*                                                                   */
/* A chunk of state objects which fits in a single granule.          */
/* Lies just after the descriptor.                                   */
/*********************************************************************/
typedef struct ismStore_memStateChunk_t
{
   ismStore_Handle_t               OwnerHandle;        /* Refers back to the owner record           */
   uint32_t                        OwnerVersion;       /* Version of the owner record. Should be    */
                                                       /* the same as the version of the owner. If  */
                                                       /* it is different then the chunk is no      */
                                                       /* longer valid and should be discarded.     */
   uint16_t                        StatesCount;
   uint16_t                        LastAddedIndex;
   ismStore_memState_t             States[1];
} ismStore_memStateChunk_t;

/*********************************************************************/
/* State Context                                                     */
/*                                                                   */
/* Used to track states for a specific owner handle. Stored in the   */
/* main memory (not in the store).                                   */
/*********************************************************************/
typedef struct ismStore_memStateContext_t
{
   ismSTORE_MUTEX_t               *pMutex;
   ismStore_Handle_t               OwnerHandle;        /* Handle of the owner record                */
   ismStore_Handle_t               hStateHead;         /* Handle of the 1st StateChunk              */
   uint32_t                        OwnerVersion;       /* Version of the owner                      */
} ismStore_memStateContext_t;

/*********************************************************************/
/* Store-Operation                                                   */
/*                                                                   */
/* An operation to be applied to the store's data. Combined into     */
/* store-transactions.                                               */
/*********************************************************************/
typedef enum
{
   Operation_Null                 = 0,                    /* Null operation                         */
   Operation_CreateRecord         = 1,                    /* Create record                          */
   Operation_DeleteRecord         = 2,                    /* Delete record                          */
   Operation_UpdateRecord         = 3,                    /* Update record                          */
   Operation_UpdateRecordAttr     = 4,                    /* Update record attribute                */
   Operation_UpdateRecordState    = 5,                    /* Update record state                    */
   Operation_CreateReference      = 6,                    /* Create reference                       */
   Operation_DeleteReference      = 7,                    /* Delete reference                       */
   Operation_UpdateReference      = 8,                    /* Update reference                       */
   Operation_UpdateRefState       = 9,                    /* Update disk reference                  */
   Operation_CreateState          = 10,                   /* Create state                           */
   Operation_DeleteState          = 11,                   /* Delete state                           */
   Operation_UpdateActiveOid      = 12,                   /* Update active OrderId on the Standby   */

   Operation_AllocateGranulesMap  = 50,                   /* Allocate granules map on the Standby   */
   Operation_CreateGranulesMap    = 51,                   /* Create granules map on the Standby     */
   Operation_SetGranulesMap       = 52                    /* Set Granules map data on the Standby   */
} ismStore_memOperationType_t;

typedef struct ismStore_memStoreOperation_t
{
   ismStore_memOperationType_t    OperationType;
   union
   {
      struct
      {
         ismStore_Handle_t         Handle;
         ismStore_Handle_t         LDHandle;
         uint16_t                  DataType;
      } CreateRecord;
      struct
      {
         ismStore_Handle_t         Handle;
      } DeleteRecord;
      struct
      {
         ismStore_Handle_t         Handle;
         uint64_t                  Attribute;
         uint64_t                  State;
      } UpdateRecord;
      struct
      {
         ismStore_Handle_t         Handle;
         ismStore_Handle_t         RefHandle;
         uint32_t                  Value;
         uint8_t                   State;
      } CreateReference;
      struct
      {
         ismStore_Handle_t         Handle;
      } DeleteReference;
      struct
      {
         ismStore_Handle_t         Handle;
         uint8_t                   State;
      } UpdateReference;
      struct
      {
         ismStore_Handle_t         Handle;
      } CreateState;
      struct
      {
         ismStore_Handle_t         Handle;
      } DeleteState;
      struct
      {
         ismStore_Handle_t         OwnerHandle;
         uint64_t                  OrderId;
      } UpdateActiveOid;
   };
} ismStore_memStoreOperation_t;

/*********************************************************************/
/* Store-Transaction                                                 */
/*                                                                   */
/* Contains a sequence of operations to be performed atomically      */
/* on the store's data.                                              */
/*********************************************************************/
typedef struct ismStore_memStoreTransaction_t
{
   uint32_t                        OperationCount;
   ismStore_GenId_t                GenId;
   uint8_t                         State;
   // The Pad is used here to make the Operations array a 64-bytes aligned.
   uint8_t                         Pad[17];
   ismStore_memStoreOperation_t    Operations[1];
} ismStore_memStoreTransaction_t;

#define ismSTORE_MEM_STORETRANSACTIONSTATE_ACTIVE       0
#define ismSTORE_MEM_STORETRANSACTIONSTATE_ROLLING_BACK 1
#define ismSTORE_MEM_STORETRANSACTIONSTATE_COMMITTING   2

/*********************************************************************/
/* Stream                                                            */
/*                                                                   */
/* A sequence of store-transactions. When a store-transaction is     */
/* committed, all store-transactions before it in the stream will    */
/* also be committed.                                                */
/* In case of a large store transaction, multiple chunks are         */
/* allocated from the store management generation.                   */
/*********************************************************************/

// forward declaration
typedef struct ismStore_memStream_t  ismStore_memStream_t ; 

#define  PERSIST_MAX_CBs  16384

typedef struct persistCBinfo_
{
  ismStore_CompletionCallback_t  pCallback;
  void                          *pContext;
  uint64_t                       MsgSqn;
  double                         TimeStamp;
  double                         deliveryTime;
  ismStore_memStream_t          *pStream;
  uint32_t                       numCBs;
} ismStore_commitCBinfo_t ; 
                        
typedef struct
{
   uint64_t                         MsgSqn;
   uint64_t                         AckSqn;
   uint64_t                         FragsCount;
   ismStore_memHAFragment_t        *pFrag;
   ismStore_memHAFragment_t        *pFragTail;
   ismStore_commitCBinfo_t          CBs[PERSIST_MAX_CBs];
   ismStore_commitCBinfo_t          curCB[1];
   uint8_t                         *lastSTflags;
   void                            *Buff;
   void                            *Buf0;
   void                            *Buf1;
   void                            *RSRV;
   double                           ct;
   double                           TimeStamp;
   uint32_t                         BuffSize;
   uint32_t                         BuffLen;
   uint32_t                         Buf0Len;
   uint32_t                         Buf1Len;
   uint32_t                         NumCBs;
   uint32_t                         NumSTs;
   uint32_t                         State;
   uint16_t                         FragSqn;
   uint16_t                         Waiting;
   uint32_t                         numFullCB;
   uint32_t                         numFullBuff;
   uint32_t                         numCommit; 
   uint32_t                         numComplete;
   uint32_t                         numPackets;
   uint32_t                         numBytes;
   uint32_t                         statCBs;
   uint32_t                         indRx;
   uint32_t                         indTx;
} ismStore_persistInfo_t ; 

#define H_STREAM_CLOSED  ((ismStore_StreamHandle_t)(-1))

struct ismStore_memStream_t
{
   ismStore_memDescriptor_t *      pDescrTranHead;
   ismStore_Handle_t               hStoreTranHead;        /* Handle of the first store-transaction    */
                                                          /* chunk in the store                       */
   ismStore_Handle_t               hStoreTranCurrent;     /* Handle of the current store-transaction  */
                                                          /* chunk in the store                       */
   ismStore_Handle_t               hStoreTranRsrv;        /* Handle of the last reserved              */
                                                          /* store-transaction chunk in the store     */
   ismStore_Handle_t               hCacheHead;            /* Handle of the first granule in the cache */
   ismSTORE_MUTEX_t                Mutex;                 /* Mutex                                    */
   ismSTORE_COND_t                 Cond;                  /* Condition variable                       */
   ismStore_memHAChannel_t        *pHAChannel;            /* High-Availability channel                */
   ismStore_StreamHandle_t         hStream;               /* Stream handle                            */
   uint32_t                        ChunksInUse;           /* Number of chunks used by the stream for  */
                                                          /* store-transaction operations             */
   uint32_t                        ChunksRsrv;            /* Number of chunks reserved by this stream */
                                                          /* for store-transaction operations         */
   uint32_t                        CacheGranulesCount;    /* Number of free granules in the cache     */
   uint32_t                        CacheMaxGranulesCount; /* Maximum number of free granules in the   */
                                                          /* cache                                    */
   uint16_t                        RefsCount;             /* Number of threads use this stream        */
   ismStore_GenId_t                ActiveGenId;           /* Active generation identifier             */
   ismStore_GenId_t                MyGenId;               /* Current stream generation identifier     */
   uint8_t                         ActiveGenIndex;        /* Active generation index                  */
   uint8_t                         MyGenIndex;            /* Current stream generation index          */
   uint8_t                         fHighPerf;             /* High performance flag                    */
   uint8_t                         fLocked;               /* Indicates if the store is locked         */
   ismStore_persistInfo_t         *pPersist;              /* Shm Persist info                         */
   struct ismStore_memStream_t    *next ;                 /* For closed streams                       */

} ;

/*********************************************************************/
/* Store Job                                                         */
/*                                                                   */
/* Contains information about job to be process by the store thread  */
/*********************************************************************/
typedef enum
{
   StoreJob_CreateGeneration      = 1,
   StoreJob_ActivateGeneration    = 2,
   StoreJob_WriteGeneration       = 3,
   StoreJob_DeleteGeneration      = 4,
   StoreJob_CompactGeneration     = 5,
   StoreJob_CheckDiskUsage        = 6,
   StoreJob_InitRsrvPool          = 7,
   StoreJob_UserEvent             = 8,
   StoreJob_IncRefGenPool         = 9,
   StoreJob_DecRefGenPool         = 10,
   StoreJob_HASendMinActiveOid    = 11,
   StoreJob_HAViewChanged         = 12,
   StoreJob_HAStandbyJoined       = 13,
   StoreJob_HAStandbyLeft         = 14,
   StoreJob_HAStandby2Primary     = 15,

   StoreJob_LastJobId             = 15
} ismStore_memJobType_t;

typedef struct ismStore_memJob_t
{
   ismStore_memJobType_t           JobType;
   union
   {
      struct
      {
         ismStore_GenId_t          GenId;
         uint8_t                   GenIndex;
      } Generation;
      struct
      {
         ismStore_memRefGenPool_t *pRefGenPool;
      } RefGenPool;
      struct
      {
         ismStore_EventType_t      EventType;
      } Event;
      struct
      {
         ismStore_Handle_t         OwnerHandle;
         uint64_t                  MinActiveOid;
      } UpdOrderId;
      struct
      {
         uint32_t                  ViewId;
         ismHA_Role_t              NewRole;
         ismHA_Role_t              OldRole;
         uint16_t                  ActiveNodesCount;
      } HAView;
   };
   struct ismStore_memJob_t       *pNextJob;

} ismStore_memJob_t;

/*********************************************************************/
/* Global                                                            */
/*                                                                   */
/* Global data for the Store component.                              */
/* This is a singleton for the entire messaging server.              */
/*********************************************************************/
typedef struct ismStore_memGeneration_t
{
   char                           *pBaseAddress;
   ismStore_Handle_t               Offset;

   uint64_t                        HAActivateSqn;
   uint64_t                        HACreateSqn;
   uint64_t                        HAWriteSqn;
   ismStore_memGranulePool_t       CoolPool[ismSTORE_GRANULE_POOLS_COUNT];
   pthread_mutex_t                 PoolMutex[ismSTORE_GRANULE_POOLS_COUNT];
   uint32_t                        PoolMaxCount[ismSTORE_GRANULE_POOLS_COUNT];
   uint32_t                        PoolMaxResrv[ismSTORE_GRANULE_POOLS_COUNT];  /* maximal size of a single reservation (e.g., message) */
   uint32_t                        PoolAlertOnCount[ismSTORE_GRANULE_POOLS_COUNT];
   uint32_t                        PoolAlertOffCount[ismSTORE_GRANULE_POOLS_COUNT];
   uint32_t                        StreamCacheMaxCount[ismSTORE_GRANULE_POOLS_COUNT];
   uint32_t                        StreamCacheBaseCount[ismSTORE_GRANULE_POOLS_COUNT];
   uint32_t                        MaxTranOpsPerGranule;  /* Maximum number of store-transaction      */
                                                          /* operations per granule                   */
   uint32_t                        MaxStatesPerGranule;   /* Maximum number of states per granule     */
   uint32_t                        MaxRefsPerGranule;     /* Maximum number of references per granule */
   uint32_t                        MaxRefStatesPerGranule;/* Maximum number of refStates per granule  */
   uint8_t                         fPoolMemAlert[ismSTORE_GRANULE_POOLS_COUNT];
   uint8_t                         PoolMutexInit;
   uint8_t                         HACreateState;         /* 0=Not sent, 1=Sent, 2=Done */
   uint8_t                         HAActivateState;       /* 0=Not sent, 1=Sent, 2=Done */
   uint8_t                         HAWriteState;          /* 0=Not sent, 1=Sent, 2=Done */
} ismStore_memGeneration_t;

typedef struct ismStore_memGranulesMap_t
{
   uint64_t                        Offset;                /* Offset of the first granule              */
   uint64_t                        Last;                  /* Last offset of the pool                  */
   uint64_t                       *pBitMap[ismSTORE_BITMAPS_COUNT]; /* Pointer to the array of bitmaps*/
   uint32_t                        BitMapSize;            /* Size of the bitmap in uint64_t entries   */
   uint32_t                        GranuleSizeBytes;      /* Granule size in bytes                    */
} ismStore_memGranulesMap_t;

typedef struct ismStore_memGenMap_t
{
   uint64_t                        MemSizeBytes;          /* Total memory size in bytes               */
   uint64_t                        DiskFileSize;          /* Disk file size in bytes                  */
   uint64_t                        HARemoteSizeBytes;     /* Remote memory size in bytes              */
   uint64_t                        PredictedSizeBytes;    /* Predicted size in bytes                  */
   uint64_t                        PrevPredictedSizeBytes;/* Previous predicted size in bytes         */
   uint64_t                        StdDevBytes;           /* Standard Deviation of records in bytes   */
   ismStore_memGranulesMap_t       GranulesMap[ismSTORE_GRANULE_POOLS_COUNT];
   ismStore_memGeneration_t       *pGen ; 
   uint32_t                        MeanRecordSizeBytes;
   uint32_t                        RecordsCount;
   uint32_t                        DelRecordsCount;
   uint8_t                         GranulesMapCount;
   uint8_t                         fCompactReady;         /* The generation is ready for compaction   */
   uint8_t                         fBitmapWarn;
   pthread_mutex_t                 Mutex;
   pthread_cond_t                  Cond;
   ismStore_memGenToken_t          GenToken;
   char                           *pHASyncBuffer;
   uint64_t                        HASyncBufferLength;
   uint64_t                        HASyncDataLength;
   ismStore_memHASyncGenState_t    HASyncState;
} ismStore_memGenMap_t;

typedef struct ismStore_memGlobal_t
{
   uint8_t                         State;
   uint8_t                         fMemValid;
   uint8_t                         fActive;
   uint8_t                         fLocked;
   char                            SharedMemoryName[NAME_MAX + 1];
   char                           *pStoreBaseAddress;
   uint64_t                        NVRAMOffset;
   uint64_t                        TotalMemSizeBytes;
   uint64_t                        RecoveryMinMemSizeBytes;
   uint64_t                        RecoveryMaxMemSizeBytes;
   uint64_t                        CompactMemThBytes;
   uint64_t                        CompactDiskThBytes;
   uint64_t                        CompactMemBytesHWM;
   uint64_t                        CompactMemBytesLWM;
   double                          StreamsCachePct;
   uint32_t                        MgmtSmallGranuleSizeBytes;
   uint32_t                        MgmtGranuleSizeBytes;
   uint32_t                        GranuleSizeBytes;
   uint32_t                        RefCtxtLocksCount;
   uint32_t                        StateCtxtLocksCount;
   uint32_t                        StoreTransRsrvOps;
   uint32_t                        RefGenPoolExtElements;
   uint32_t                        RefGenPoolHWM;
   uint32_t                        RefGenPoolLWM;
   uint16_t                        MgmtMemPct;
   uint16_t                        MgmtSmallGranulesPct;
   uint16_t                        MgmtAlertOnPct;
   uint16_t                        MgmtAlertOffPct;
   uint16_t                        GenAlertOnPct;
   uint16_t                        CompactDiskHWM;
   uint16_t                        CompactDiskLWM;
   uint16_t                        OwnerLimitPct;
   uint16_t                        PersistRecoveryFlags;
   ismStore_GenId_t                PersistCreatedGenId;

   ismStore_memGeneration_t        MgmtGen;
   ismStore_memGeneration_t        InMemGens[ismSTORE_MAX_INMEM_GENS];
   uint8_t                         InMemGensCount;

   uint8_t                         RsrvPoolState;         /* 0=Not assigned, 1=Assigned, 2=HA Pending, 3=Initialized, 4=Attached */
   uint8_t                         RsrvPoolId;
   uint8_t                         RsrvPoolMutexInit;
   pthread_mutex_t                 RsrvPoolMutex;
   pthread_cond_t                  RsrvPoolCond;
   ismStore_memGranulePool_t       RsrvPool;
   uint64_t                        RsrvPoolHASqn;

   uint32_t                        RefCtxtMutexInit;
   uint32_t                        StateCtxtMutexInit;
   uint32_t                        RefCtxtIndex;
   uint32_t                        StateCtxtIndex;
   ismSTORE_MUTEX_t               **pRefCtxtMutex;
   ismSTORE_MUTEX_t               **pStateCtxtMutex;
   pthread_mutex_t                 CtxtMutex;
   uint8_t                         CtxtMutexInit;

   ismStore_memRefGenPool_t       *pRefGenPool;

   // Store thread information
   ism_threadh_t                   ThreadId;
   pthread_mutex_t                 ThreadMutex;
   pthread_cond_t                  ThreadCond;
   uint8_t                         ThreadMutexInit;
   uint8_t                         fThreadGoOn;
   uint8_t                         fThreadUp;
   uint8_t                         fWithinStart;
   uint32_t                        ThreadJobsHistory[StoreJob_LastJobId + 1];
   ismStore_memJob_t              *pThreadFirstJob;
   ismStore_memJob_t              *pThreadLastJob;

   // List of active streams used by the store.
   ismStore_memStream_t           *dStreams;
   ismStore_memStream_t          **pStreams;
   uint32_t                        StreamsCount;
   uint32_t                        StreamsSize;
   uint32_t                        StreamsMinCount;
   uint32_t                        StreamsUpdCount;
   pthread_mutex_t                 StreamsMutex;
   pthread_cond_t                  StreamsCond;
   uint8_t                         StreamsMutexInit;

   ismStore_StreamHandle_t         hInternalStream;

   // High-Availability information
   ismStore_memHAInfo_t            HAInfo;

#if ismSTORE_ADMIN_CONFIG
   // ISM admin dynamic configuration handle
   ism_config_t                   *hConfig;
#endif

   // Store disk information
   uint8_t                         fDiskUp;
   uint8_t                         DiskMutexInit;
   pthread_mutex_t                 DiskMutex;
   char                            DiskRootPath[PATH_MAX + 1];
   size_t                          DiskBlockSizeBytes;
   uint32_t                        DiskTransferSize;
   uint16_t                        DiskAlertOnPct;
   uint16_t                        DiskAlertOffPct;
   uint8_t                         fDiskAlertOn;
   uint8_t                         fCompactDiskAlertOn;

   uint8_t                         fCompactMemAlertOn;
   uint8_t                         fEnablePersist;
   uint8_t                         fReuseSHM;
   uint8_t                         PersistThreadPolicy;
   uint8_t                         PersistAsyncThreads;
   uint8_t                         PersistAsyncOrdered;
   uint8_t                         PersistHaTxThreads;
   uint8_t                         PersistRecoverFromV12;
   ismStore_AsyncCBStatsMode_t     AsyncCBStatsMode;
   mode_t                          PersistedFileMode;
   mode_t                          PersistedDirectoryMode;
   uint32_t                        PersistCbHwm;
   uint32_t                        PersistBuffSize;
   uint64_t                        PersistFileSize;
   char                            PersistRootPath[PATH_MAX + 1];

   // List of (active and disk) generations used by the store.
   // The list is used to track over the chunks of each generations.
   // Since the list is not persistent, it is re-built during the recovery process
   uint32_t                        GensMapCount;
   uint32_t                        GensMapSize;
   ismStore_memGenMap_t          **pGensMap;

   ismPSTOREEVENT                  OnEvent;
   void                           *pEventContext;

   char                            LockFileName[NAME_MAX + 1];
   int                             LockFileDescriptor;

   uint32_t                        OwnerGranulesLimit;
   uint32_t                        OwnerCount[ISM_STORE_NUM_REC_TYPES];
   uint32_t                        RefSearchCacheSize;
   uint32_t                        RefChunkHWMpct;
   uint32_t                        RefChunkLWMpct;
   uint32_t                        RefChunkHWM;
   uint32_t                        RefChunkLWM;
   char                            zero4cmp[1024] ; // MUST be at least regular gen granule size
} ismStore_memGlobal_t;

#define ismSTORE_STATE_CLOSED            0x0
#define ismSTORE_STATE_INIT              0x1
#define ismSTORE_STATE_RESTORING         0x2
#define ismSTORE_STATE_RESTORED          0x3
#define ismSTORE_STATE_STANDBY           0x4
#define ismSTORE_STATE_RECOVERY          0x5
#define ismSTORE_STATE_ACTIVE            0x6
#define ismSTORE_STATE_TERMINATING       0x7
#define ismSTORE_STATE_DISKERROR         0x8
#define ismSTORE_STATE_ALLOCERROR        0x9

/*********************************************************************/
/* Internal function prototypes                                      */
/*********************************************************************/
int32_t ism_store_memStart(void);

int32_t ism_store_memTerm(
                   uint8_t fHAShutdown);

int32_t ism_store_memDrop(void);

int32_t ism_store_memRegisterEventCallback(
                   ismPSTOREEVENT callback,
                   void *pContext);

int32_t ism_store_memValidate(void);

void *ism_store_memMapHandleToAddress(
                   ismStore_Handle_t handle);

ismStore_memGeneration_t *ism_store_memGetGenerationById(
                   ismStore_GenId_t genId);

void ism_store_memPreparePool(
                   ismStore_GenId_t genId,
                   ismStore_memGeneration_t *pGen,
                   ismStore_memGranulePool_t *pPool,
                   uint8_t poolId,
                   uint8_t fNew);

void ism_store_memForceWriteBack(
                   void *address,
                   size_t length);

int32_t ism_store_memCreateGranulesMap(
                   ismStore_memGenHeader_t *pGenHeader,
                   ismStore_memGenMap_t *pGenMap,
                   uint8_t fBuildBitMapFree);

void ism_store_memSetGenMap(
                   ismStore_memGenMap_t *pGenMap,
                   uint8_t bitMapIndex,
                   ismStore_Handle_t offset,
                   uint32_t dataLength);

void ism_store_memResetGenMap(
                   ismStore_Handle_t handle);

int32_t ism_store_memGetGenIdOfHandle(
                   ismStore_Handle_t handle,
                   ismStore_GenId_t *pGenId);

int32_t ism_store_memGetStatistics(
                   ismStore_Statistics_t *pStatistics);

int32_t ism_store_memReturnPoolElements(
                   ismStore_memStream_t *pStream,
                   ismStore_Handle_t handle, uint8_t f2Cool);

int32_t ism_store_memOpenStream(
                   ismStore_StreamHandle_t *phStream, 
                   uint8_t fNoneCrit);


int32_t ism_store_memCloseStream(
                   ismStore_StreamHandle_t hStream);

int32_t ism_store_memReserveStreamResources(
                   ismStore_StreamHandle_t hStream,
                   ismStore_Reservation_t *pReservation);

int ism_store_memGetAsyncCBStats(uint32_t *pTotalReadyCBs, uint32_t *pTotalWaitingCBs,
                                 uint32_t *pNumThreads,
                                 ismStore_AsyncThreadCBStats_t *pCBThreadStats);

int32_t ism_store_memCancelResourceReservation(ismStore_StreamHandle_t hStream);

int32_t ism_store_memStartTransaction(ismStore_StreamHandle_t hStream, int *fIsActive);

int32_t ism_store_memCancelTransaction(ismStore_StreamHandle_t hStream);

int32_t ism_store_memGetStreamOpsCount(
                   ismStore_StreamHandle_t hStream,
                   uint32_t *pOpsCount);

int32_t ism_store_getReferenceStatistics(
                   ismStore_Handle_t hOwnerHandle,
                   ismStore_ReferenceStatistics_t *pRefStats);

int32_t ism_store_memOpenReferenceContext(
                   ismStore_Handle_t hOwnerHandle,
                   ismStore_ReferenceStatistics_t *pRefStats,
                   void **phRefCtxt);

int32_t ism_store_memCloseReferenceContext(
                   void *hRefCtxt);

int32_t ism_store_memClearReferenceContext(
                   ismStore_StreamHandle_t hStream,
                   void *hRefCtxt);

ismStore_Handle_t ism_store_memMapReferenceContextToHandle(
                   void *hRefCtxt);

int32_t ism_store_memOpenStateContext(
                   ismStore_Handle_t hOwnerHandle,
                   void **phStateCtxt);

int32_t ism_store_memCloseStateContext(
                   void *hStateCtxt);

ismStore_Handle_t ism_store_memMapStateContextToHandle(
                   void *hStateCtxt);

ismStore_memRefGen_t *ism_store_memAllocateRefGen(
                   ismStore_memReferenceContext_t *pRefCtxt);

void ism_store_memFreeRefGen(
                   ismStore_memReferenceContext_t *pRefCtxt,
                   ismStore_memRefGen_t *pRefGen);

int32_t ism_store_memEndStoreTransaction(
                   ismStore_StreamHandle_t hStream,
                   uint8_t fCommit,
                   ismStore_CompletionCallback_t pCallback,
                   void *pContext);

int32_t ism_store_memAssignRecordAllocation(
                   ismStore_StreamHandle_t hStream,
                   const ismStore_Record_t *pRecord,
                   ismStore_Handle_t *pHandle);

int32_t ism_store_memFreeRecordAllocation(
                   ismStore_StreamHandle_t hStream,
                   ismStore_Handle_t handle);

int32_t ism_store_memUpdateRecord(
                   ismStore_StreamHandle_t hStream,
                   ismStore_Handle_t handle,
                   uint64_t attribute,
                   uint64_t state,
                   uint8_t flags);

int32_t ism_store_memAddReference(
                   ismStore_StreamHandle_t hStream,
                   void *hRefCtxt,
                   const ismStore_Reference_t *pReference,
                   uint64_t minimumActiveOrderId,
                   uint8_t fAutoCommit,
                   ismStore_Handle_t *pHandle);

int32_t ism_store_memUpdateReference(
                   ismStore_StreamHandle_t hStream,
                   void *hRefCtxt,
                   ismStore_Handle_t handle,
                   uint64_t orderId,
                   uint8_t state,
                   uint64_t minimumActiveOrderId,
                   uint8_t fAutoCommit);

int32_t ism_store_memDeleteReference(
                   ismStore_StreamHandle_t hStream,
                   void *hRefCtxt,
                   ismStore_Handle_t handle,
                   uint64_t orderId,
                   uint64_t minimumActiveOrderId,
                   uint8_t fAutoCommit);

int32_t ism_store_memPruneReferences(
                   ismStore_StreamHandle_t hStream,
                   void *hRefCtxt,
                   uint64_t minimumActiveOrderId);

int32_t ism_store_memAddState(
                   ismStore_StreamHandle_t hStream,
                   void *hStateCtxt,
                   const ismStore_StateObject_t *pState,
                   ismStore_Handle_t *pHandle);

int32_t ism_store_memDeleteState(
                   ismStore_StreamHandle_t hStream,
                   void *hStateCtxt,
                   ismStore_Handle_t handle);

void ism_store_memDiskWriteComplete(
                   ismStore_GenId_t genId,
                   int32_t retcode,
                   ismStore_DiskGenInfo_t *pDiskGenInfo,
                   void *pContext);

int32_t ism_store_memCreateGeneration(
                   uint8_t genIndex, ismStore_GenId_t genId0);

#define LOCKSTORE_CALLER_SYNC 0x1
#define LOCKSTORE_CALLER_STOR 0x2
#define LOCKSTORE_CALLER_PRST 0x4

int  ism_store_memLockStore(int waitMilli, int caller);

void ism_store_memUnlockStore(int caller);

void ism_store_memAddJob(
                   ismStore_memJob_t *pJob);

char *ism_store_memB2H(
                   char *pDst,
                   unsigned char *pSrc,
                   int len);

int ism_store_memGetGenSizes(uint64_t *mgmSizeBytes, uint64_t *genSizeBytes);
ismStore_memGeneration_t *ism_store_memInitGenStruct(uint8_t genIndex);
void ism_store_memSetStoreState(int state, int lock);

#endif /* __ISM_STORE_MEMORY_DEFINED */
/*********************************************************************/
/* End of storeMemory.h                                              */
/*********************************************************************/
