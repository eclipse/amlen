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

///////////////////////////////////////////////////////////////////////////////
///  @file queueCommon.h
///  @brief 
///    Common structures and function prototypes for Simple, Intermediate
///    and multiConsumer queues
///  @remarks 
///    The ISM Engine supports a number of different types of queues. Each of
///    these queues provide different levels of functionality but the different
///    implementations are hidden behind the common Queue structure defined
///    in this header file.
///    @par
///    Use the following macro to call the appropriate queue function for the
///    type of queue.
///    @see ieq_create
///    @see ieq_delete
///    @see ieq_drain
///    @see ieq_dump
///    @see ieq_getStats
///    @see ieq_put
///    @see ieq_initWaiter
///    @see ieq_acknowledge
///    @see ieq_acknowledgeBatch
///    @see ieq_completeAckBatch
///    @see ieq_markMessageGotInTran
///    
///////////////////////////////////////////////////////////////////////////////
#ifndef __ISM_ENGINE_QUEUECOMMON_DEFINED
#define __ISM_ENGINE_QUEUECOMMON_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "policyInfo.h"      // Engine policy information header file
#include "engineSplitList.h" // For expiry reaper list linkage structure
#include "exportResources.h"
#include "engineAsync.h"

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Parameter on the put function call which defines if the engine
///    should increment the reference count for the message or inherit it from
///    the caller.
///////////////////////////////////////////////////////////////////////////////
typedef enum tag_ieqMsgInputType_t
{
    IEQ_MSGTYPE_REFCOUNT,     ///< Use provided message, incrementing reference count
    IEQ_MSGTYPE_INHERIT       ///< Use provided message, inherit callers reference count
}  ieqMsgInputType_t;


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Used for passing options to ieq_createQ
///////////////////////////////////////////////////////////////////////////////
typedef enum 
{
    /// Default 
    ieqOptions_DEFAULT                = 0,
    /// No values
    ieqOptions_NONE                   = 0,
    /// Processing store recovery
    ieqOptions_IN_RECOVERY            = 0x01000000,
    /// This is a queue backing up a subscription
    ieqOptions_SUBSCRIPTION_QUEUE     = 0x02000000,
    /// This is a temporary queue, not persisted
    ieqOptions_TEMPORARY_QUEUE        = 0x04000000,
    /// This is a forwarding queue for delivery of messages to a remote server
    ieqOptions_REMOTE_SERVER_QUEUE    = 0x08000000,
    /// Queue is currently being imported
    ieqOptions_IMPORTING              = 0x10000000,
    /// There will only ever be a single consumer on this queue (non-shared sub)
    ieqOptions_SINGLE_CONSUMER_ONLY   = 0x00000001,
} ieqOptions_t;

#define ieqOptions_PUBSUB_QUEUE_MASK (ieqOptions_SUBSCRIPTION_QUEUE | ieqOptions_REMOTE_SERVER_QUEUE)

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Used for passing options to ieq_put
///////////////////////////////////////////////////////////////////////////////
typedef enum 
{
    /// No values
    ieqPutOptions_NONE                 = 0,
    ieqPutOptions_RETAINED             = 0x00000001, ///< The message is being delivered as a retained message
    ieqPutOptions_IGNORE_REJECTNEWMSGS = 0x20000000, ///< Ignore the max message count on subscriptions specifying RejectNewMessages (for reliable forwarder)
    ieqPutOptions_THREAD_LOCAL_MESSAGE = 0x40000000, ///< The message being published is only visible on the local thread
    ieqPutOptions_SET_ORDERID          = 0x80000000, ///< Internal use only (only works with intermediateQ!)
} ieqPutOptions_t;

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Used for passing options to ieq_setStats
///////////////////////////////////////////////////////////////////////////////
typedef enum
{
    ieqExpiryReapRC_OK           = 0,             ///< Successfully scanned queue
    ieqExpiryReapRC_RemoveQ      = 1,             ///< Please remove queue from expiry List
    ieqExpiryReapRC_NoExpiryLock = 2,             ///< Couldn't get expiry lock
} ieqExpiryReapRC_t;

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Result of expiry reaping
///////////////////////////////////////////////////////////////////////////////
typedef enum
{
    ieqSetStats_UPDATE_PUTSATTEMPTED = 0, ///< Update the the PutsAttempted statistic only
    ieqSetStats_RESET_ALL            = 1, ///< Reset all statistics to zero
} ieqSetStatsType_t;

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Internal Stats about a consumer on a subscription
///////////////////////////////////////////////////////////////////////////////

typedef struct tag_ieqConsumerStats_t {
    char *clientId;
    uint64_t messagesInFlightToConsumer;
    uint64_t messagesInFlightToClient;   //NB: Not all messages are tracked for all clients,
                                         //      e.g. for non-durable JMS subs, they won't contribute to this number
    uint64_t maxInflightToClient;        //Max engine will allow to be sent to client (not capped for JMS)
    uint64_t inflightReenable;           //When inflight falls to this value, engine will re-enable delivery
    iewsWaiterStatus_t consumerState;    //Engine's consumer state - can determine whether the protocol wants the consumer enabled
    bool sessionDeliveryStarted;         //Engine's record of whether the protocol has asked us to start
    bool sessionDeliveryStopping;        //Engine's record of whether the protocol has asked us to stop
    bool sessionFlowControlPaused;       //Whether the session has been temporarily disabled by the engine
    bool consumerFlowControlPaused;      //Whether the consumer has been temporarily paused by the engine
    bool mqttIdsExhausted;               //Whether the cllientstate hMsgDlbyInfo thinks ids are exhausted
} ieqConsumerStats_t;

///  @name Q CallInterface
///  @brief 
///    The following sets of types are used to provide a structure 
///    of function pointers which direct the function call to the 
///    correct for function accoring to the type of queue.
///  @{

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Type of the common create function
///  @param[in]  const char *            - Name (optional)
///  @param[in]  ieqOptions_t            - Options
///  @param[in]  iepiPolicyInfo_t        - Policy Information
///  @param[in]  ismStore_Handle_t       - Definition store handle
///  @param[in]  ismStore_Handle_t       - Properties store handle
///  @param[in]  iereResourceSetHandle_t - ResourceSet handle
///  @param[out] ismQHandle_t            - Ptr to Q that has been created (if
///                                        returned ok)
///  @return                             - OK on success or an ISMRC error code
typedef int32_t (*ieqCreate_t)(ieutThreadData_t *, const char *, ieqOptions_t, iepiPolicyInfo_t *, ismStore_Handle_t, ismStore_Handle_t, iereResourceSetHandle_t, ismQHandle_t *);

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Type of the common deleteQ function
///  @param[in] ismQHandle_t       - Ptr to Q to be deleted
///  @param[in] bool               - Only free memory, no persistent updates
///  @return                       - OK on success or an ISMRC error code
typedef int32_t (*ieqDelete_t)(ieutThreadData_t *, ismQHandle_t *, bool);

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Type of the common drainQ function
///  @param[in] ismQHandle_t       - Queue
///  @return                       - OK on success or an ISMRC error code
typedef int32_t (*ieqDrain_t)(ieutThreadData_t *, ismQHandle_t);

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Type of the common debugQ function
///  @param[in]     ismQHandle_t               Queue handle
///  @param[in]     ismEngine_DebugOptions_t   Debug options
///  @param[in]     ismEngine_DebugHdl_t       Debug file handle (may be NULL)
typedef void (*ieqDebug_t)(ieutThreadData_t *, ismQHandle_t, ismEngine_DebugOptions_t, ismEngine_DebugHdl_t *);

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Type of the common dumpQ function
///  @param[in]     ismQHandle_t               Queue handle
///  @param[in]     iedmDumpHandle_t           Dump handle
typedef void (*ieqDump_t)(ieutThreadData_t *, ismQHandle_t, iedmDumpHandle_t);

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Type of the common getStats function
///  @param[in] ismQHandle_t                 Queue
///  @param[out] ismEngine_QueueStatistics_t Statistics data
typedef void (*ieqGetStats_t)(ieutThreadData_t *, ismQHandle_t, ismEngine_QueueStatistics_t *);

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Type of the common setPutsAttemptedStat function
///  @param[in] ismQHandle_t                 Queue
///  @param[in] ismEngine_QueueStatistics_t  Statistics data from which to take value
///  @param[in] ieqSetStatType_t             Type of statistics setting required
typedef void (*ieqSetStats_t)(ismQHandle_t, ismEngine_QueueStatistics_t *, ieqSetStatsType_t);

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Type of the common checkAvailableMsgs function
///  @param[in]   ismQHandle_t          Queue

typedef int32_t (*ieqCheckAvailableMsgs_t)(ismQHandle_t, ismEngine_Consumer_t *);

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Type of the common put function
///  @param[in] ismQHandle_t        - Q to put to
///  @param[in] ieqPutOptions_t     - Queue Put options
///  @param[in] ismEngine_Message_t - message to put
///  @param[in] ieqMsgInputType_t   - IEQ_MSGTYPE_REFCOUNT (re-use & increment refcount)
///                                   or IEQ_MSGTYPE_INHERIT (re-use)
///  @return                        - OK on success or an ISMRC error code
typedef int32_t (*ieqPut_t)(ieutThreadData_t *, ismQHandle_t, ieqPutOptions_t, ismEngine_Transaction_t *, ismEngine_Message_t *, ieqMsgInputType_t);

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Type of the common msg rehydrate function
///  @param[in] ismQHandle_t        - Q to put to
///  @param[in] ismEngine_Message_t - message to put
///  @param[in] ismStore_Handle_t   - Handle to store message record
///  @param[in] ismStore_Reference_t- Store message reference data
///  @return                        - OK on success or an ISMRC error code
typedef int32_t (*ieqRehydrateMsg_t)(ieutThreadData_t *, ismQHandle_t, ismEngine_Message_t *, ismEngine_RestartTransactionData_t *, ismStore_Handle_t, ismStore_Reference_t *);

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Type of the common delivery ID rehydrate function
///  @param[in] Qhdl               - Q to put to
///  @param[in] hMsgDelInfo        - handle of message-delivery information
///  @param[in] hMsgRef            - handle of message reference
///  @param[in] deliveryId         - Delivery id
///  @param[out] ppnode            - Pointer to return queue node
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
typedef int32_t (*ieqRehydrateDeliveryId_t)(ieutThreadData_t *, ismQHandle_t, iecsMessageDeliveryInfoHandle_t, ismStore_Handle_t, uint32_t, void **);

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Type of the common initWaiter function
///  @param[in] ismQHandle_t         - Queue
///  @param[in] ismEngine_Consumer_t - Consumer to initialise
///  @return                         - OK
typedef int32_t (*ieqInitWaiter_t)(ieutThreadData_t *, ismQHandle_t, ismEngine_Consumer_t *);

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Type of the common termWaiter function
///  @param[in] ismQHandle_t         - Queue
///  @param[in] consumer           - consumer being removed///  @return                         - OK
typedef int32_t (*ieqTermWaiter_t)(ieutThreadData_t *, ismQHandle_t, ismEngine_Consumer_t *);

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Type of the common checkWaiters function
///  @param[in] ieutThreadData_t * - Thread Data structure
///  @param[in] ismQHandle_t       - Queue
///  @return                       - OK on success or an ISMRC error code
typedef int32_t (*ieqCheckWaiters_t)(ieutThreadData_t *, ismQHandle_t, ismEngine_AsyncData_t *);


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Type of the common acknowledge function
///  @param[in] ismQHandle_t         - Queue handle
///  @param[in] session              - Session
///  @param[in] ismEngine_Transaction_t * - Transaction (optional)
///  @param[in] void *               - Queue node pointer
///  @param[in] uint32_t             - Options
///  @param[in] ismEngine_BatchAckState_t * - Acknowledge state
///  @return                         - OK on success or an ISMRC error code
typedef int32_t (*ieqAcknowledge_t)( ieutThreadData_t *
                                   , ismQHandle_t
                                   , ismEngine_Session_t *
                                   , ismEngine_Transaction_t *
                                   , void *
                                   , uint32_t
                                   , ismEngine_AsyncData_t *);

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Type of the common acknowledge function
///  @param[in] ieutThreadData_t *   - Thread Structure
///  @param[in] ismQHandle_t         - Queue handle
///  @param[in] void *               - Queue node pointer
///  @param[in]  ismEngine_RelinquishType_t - ack or nack these messages
///  @param[out] uint32_t *          - storeOpCount
///  @return                         - OK on success or an ISMRC error code
typedef int32_t (*ieqRelinquish_t)( ieutThreadData_t *
                                  , ismQHandle_t
                                  , void *
                                  , ismEngine_RelinquishType_t
                                  , uint32_t *);

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Type of the common ieqPrepareAck function (1st phase of acknowledge)
///  @param[in] ismQHandle_t         - Queue handle
///  @param[in] session              - Session
///  @param[in] ismEngine_Transaction_t * - Transaction (optional)
///  @param[in] void *               - Queue node pointer
///  @param[in] uint32_t             - Options
///  @return                         - OK on success or an ISMRC error code
typedef void (*ieqPrepareAck_t)(ieutThreadData_t *, ismQHandle_t, ismEngine_Session_t *, ismEngine_Transaction_t *, void *, uint32_t, uint32_t *);

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Type of the common ieqProcessAck_t function (2nd phase of acknowledge)
///  @param[in] ismQHandle_t         - Queue handle
///  @param[in] session              - Session
///  @param[in] ismEngine_Transaction_t * - Transaction (optional)
///  @param[in] void *               - Queue node pointer
///  @param[in] uint32_t             - Options
///  @param[in] ismEngine_BatchAckState_t * - Acknowledge state
///  @return                         - OK on success or an ISMRC error code
/// @remark
/// If phMsgToUnstore == NULL on input then this function uses lazy removal
/// (So the caller must either supply phMsgToUnstore and later call iest_finishUnstoreMessages()
///  OR must call iest_checkLazyMessages() / iest_store_(async)commit before next potential use
/// of lazy removal
typedef int32_t (*ieqProcessAck_t)(ieutThreadData_t *, ismQHandle_t, ismEngine_Session_t *, ismEngine_Transaction_t *, void *, uint32_t,
                                   ismStore_Handle_t *, bool *, ismEngine_BatchAckState_t *, ieutThreadData_t **);


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Type of the common completeAckBatch function
///  @param[in] ismQHandle_t         - Queue handle
///  @param[in] session              - Session
///  @param[in] ismEngine_BatchAckState_t * - Acknowledge state
///  @return                         - none
typedef void (*ieqCompleteAckBatch_t)(ieutThreadData_t *, ismQHandle_t, ismEngine_Session_t *, ismEngine_BatchAckState_t *);

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Type of the common getDefnHdl function
///  @param[in]  ismQHandle_t        - Queue Handle
///  @return                         - Store handle
typedef ismStore_Handle_t (*ieqGetDefnHdl_t)(ismQHandle_t);

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Type of the common getPropsHdl function
///  @param[in]  ismQHandle_t        - Queue Handle
///  @return                         - Store handle
typedef ismStore_Handle_t (*ieqGetPropsHdl_t)(ismQHandle_t);

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Type of the common setPropsHdl function
///  @param[in]  ismQHandle_t        - Queue Handle
///  @param[in]  ismStore_Handle_t   - Properties Store Handle
///  @return                         - OK on success or an ismRC error code
typedef void (*ieqSetPropsHdl_t)( ismQHandle_t,
                                  ismStore_Handle_t);

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Type of the common updateProperties function
///  @param[in]  ismQHandle_t            - Queue Handle
///  @param[in]  char *                  - Queue Name
///  @param[in]  ieqOptions_t            - Queue Options
///  @param[in]  ismStore_Handle_t       - Properties store handle
///  @param[in]  iereResourceSetHandle_t - ResourceSet handle
///  @return                             - OK on success or an ismRC error code
typedef int32_t (*ieqUpdateProperties_t)(ieutThreadData_t *, ismQHandle_t, const char *, ieqOptions_t QOptions, ismStore_Handle_t, iereResourceSetHandle_t);

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Type of the common completeRehydrate function
///  @param[in]  ismQHandle_t        - Queue Handle
///  @return                         - OK on success or an ismRC error code
typedef int32_t (*ieqCompleteRehydrate_t)(ieutThreadData_t *, ismQHandle_t);

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Update the in-memory queue and optionally store record as deleted
///  @param[in] Qhdl               - Queue to be marked as deleted
///  @param[in] updateStore        - Whether to update the state of the store record
///  @return                       - OK on success or an ISMRC error code
typedef int32_t (*ieqMarkQDeleted_t)(ieutThreadData_t *, ismQHandle_t, bool);

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Is the queue marked as deleted?
///  @param[in] Qhdl               - Queue to be marked as deleted
///  @return                       - true if deleted else false
typedef bool (*ieqIsDeleted_t)(ismQHandle_t);

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Is the queue marked as deleted?
///  @param[in] Qhdl               - Queue which contains the message that was got
///  @param[in] OrderId            - OrderId of the message that was got as part of the tran
///  @param[in] Tran               - transaction to add the message to
///  @return                       -  OK on success or an ISMRC error code
typedef int32_t (*ieqMarkMessageGotInTran_t)( ieutThreadData_t *
                                            , ismQHandle_t
                                            , uint64_t
                                            , ismEngine_RestartTransactionData_t *);


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Reduce the usecount for a queue. Some queues need complex life cycles with
///  usecounting. For such queues, decrement the usecount.
///
///  @param[in] Qhdl               - Queue which has its use count
typedef void (*ieq_reducePreDeleteCount_t)(ieutThreadData_t *, ismQHandle_t);

///////////////////////////////////////////////////////////////////////////////
///  @brief
///   Some (MQTT) queues track the number of inflight messages and don't go away
///   until all the messages have been acked. This call tells the queue to
///   forget the unacked messages.
///
///  @param[in] Qhdl               - Queue
typedef void (*ieqForgetInflightMsgs_t)(ieutThreadData_t *, ismQHandle_t);

//////////////////////////////////////////////////////////////////////////////
/// @brief
///   Say only messages inflight that must have their delivery completed even
/// after an unsub (i.e. mqtt qos=1 or 2) messages should be delivered from the queue
typedef bool (*ieqRedeliverOnly_t)(ieutThreadData_t *, ismQHandle_t);

//////////////////////////////////////////////////////////////////////////////
/// @brief
///   discard older messages to make space for newer ones
/// after an unsub (i.e. mqtt qos=1 or 2) messages should be delivered from the queue
typedef void (*ieqReclaimSpace_t)(ieutThreadData_t *, ismQHandle_t, bool);

//////////////////////////////////////////////////////////////////////////////
/// @brief
///  Remove from MDR table and the queue all messages for this client
typedef void (*ieqRelinquishAll_t)(ieutThreadData_t *, ismQHandle_t, iecsMessageDeliveryInfoHandle_t, ismEngine_RelinquishType_t);

//////////////////////////////////////////////////////////////////////////////
/// @brief
/// After rehydration, delete this queue if it is no longer required
typedef void (*ieqRemoveIfUnneeded_t)(ieutThreadData_t *, ismQHandle_t);

//////////////////////////////////////////////////////////////////////////////
/// @brief
/// Scan a queue removing any expired messages
typedef ieqExpiryReapRC_t (*ieqReapExpiredMsgs_t)(ieutThreadData_t *, ismQHandle_t, uint32_t, bool, bool);

//////////////////////////////////////////////////////////////////////////////
/// @brief
/// Record that a message expired
typedef void (*ieqMessageExpired_t)(ieutThreadData_t *, ismQHandle_t);


//////////////////////////////////////////////////////////////////////////////
/// @brief
/// Export buffered messages. Does not include messages in transactions
///                           Does not include messages in flight on multiconsumerQ
///                               (we don't know which client they are inflight for)
typedef int32_t (*ieqExportMessages_t)(ieutThreadData_t *, ismQHandle_t, ieieExportResourceControl_t *);

//////////////////////////////////////////////////////////////////////////////
/// @brief
/// Finish the importing of a queue
typedef int32_t (*ieqCompleteImport_t)(ieutThreadData_t *, ismQHandle_t);


//////////////////////////////////////////////////////////////////////////////
/// @brief
/// Get stats for consumers on a queue
typedef int32_t (*ieqGetConsumerStats_t)(ieutThreadData_t *, ismQHandle_t, iempMemPoolHandle_t, size_t *, ieqConsumerStats_t consDataArray[]);



/// Structure used to define the common Queue interface
typedef struct tag_ieqInterface_t 
{
  ieqCreate_t                 createQ;
  ieqDelete_t                 deleteQ;
  ieqDrain_t                  drainQ;
  ieqDump_t                   dumpQ;
  ieqGetStats_t               getStats;
  ieqSetStats_t               setStats;
  ieqCheckAvailableMsgs_t     checkAvailableMsgs;
  ieqPut_t                    put;
  ieqRehydrateMsg_t           rehydrateMsg;
  ieqRehydrateDeliveryId_t    rehydrateDeliveryId;
  ieqInitWaiter_t             initWaiter;
  ieqTermWaiter_t             termWaiter;
  ieqCheckWaiters_t           checkWaiters;
  ieqAcknowledge_t            acknowledge;
  ieqRelinquish_t             relinquish;
  ieqPrepareAck_t             prepareAck;
  ieqProcessAck_t             processAck;
  ieqCompleteAckBatch_t       completeAckBatch;
  ieqGetDefnHdl_t             getDefnHdl;
  ieqGetPropsHdl_t            getPropsHdl;
  ieqSetPropsHdl_t            setPropsHdl;
  ieqUpdateProperties_t       updateProperties;
  ieqCompleteRehydrate_t      completeRehydrate;
  ieqMarkQDeleted_t           markQDeleted;
  ieqIsDeleted_t              isDeleted;
  ieqMarkMessageGotInTran_t   markMsgTranGot;
  ieq_reducePreDeleteCount_t  reduceCount;
  ieqForgetInflightMsgs_t     forgetInflightMsgs;
  ieqRedeliverOnly_t          redeliverOnly;
  ieqReclaimSpace_t           reclaimSpace;
  ieqRelinquishAll_t          relinquishAll;
  ieqRemoveIfUnneeded_t       removeIfUnneeded;
  ieqReapExpiredMsgs_t        reapExpiredMsgs;
  ieqMessageExpired_t         messageExpired;
  ieqExportMessages_t         exportMessages;
  ieqCompleteImport_t         completeImport;
  ieqGetConsumerStats_t       getConsumerStats;
} ieqInterface_t;


//Each queue type has a structure that contains some expiry data that is opaque to the queue
typedef struct tag_iemeQueueExpiryData_t   *iemeQueueExpiryDataHandle_t;      // defined in messageExpiryInternal.h

//*********************************************************************
/// @brief Common queue structure header
///
/// This structure contains the common queue header which all ISM queues
/// must share at the start of their structure definitions.
//*********************************************************************
typedef struct ismEngine_Queue_t
{
    char                                 StrucId[4];     ///< Eyecatcher "EQUE"
    ismQueueType_t                       QType;          ///< Type of queue
    iepiPolicyInfoHandle_t               PolicyInfo;     ///< Policy information for this queue
    const ieqInterface_t                 *pInterface;    ///< QInterface ptr
    char                                 *QName;         ///< Name of this queue (or associated subscription)
    ieutSplitListLink_t                  expiryLink;     ///< Linkage for queues in expiry reaper list
    volatile iemeQueueExpiryDataHandle_t QExpiryData;    ///< Info about earliest expiring messages on this queue
    iereResourceSetHandle_t              resourceSet;    ///< The resource set to which this queue belongs
    bool                                 informOnEmpty;  ///< call the engine when the queue is drained
} ismEngine_Queue_t;

#define ismENGINE_QUEUE_STRUCID "EQUE"

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_ismEngine_Queue_t(__file)\
    iedm_descriptionStart(__file, ismEngine_Queue_t, StrucId, ismENGINE_QUEUE_STRUCID);\
    iedm_describeMember(char [4],           StrucId);\
    iedm_describeMember(ismQueueType_t,     QType);\
    iedm_describeMember(iepiPolicyInfo_t *, PolicyInfo);\
    iedm_describeMember(ieqInterface_t *,   pInterface);\
    iedm_describeMember(char *,             QName);\
    iedm_describeMember(iemeHashSetLink_t,  expiryLink);\
    iedm_describeMember(volatile iemeQueueExpiryDataHandle_t, QExpiryData);\
    iedm_describeMember(iereResourceSet_t *, resourceSet);\
    iedm_describeMember(bool,               informOnEmpty);\
    iedm_descriptionEnd;

// Allow us to force all intermediate queue requests to instead use multiConsumer queue
#ifdef ENGINE_FORCE_INTERMEDIATE_TO_MULTI
/// Macro used to invoke create for the supplied Queue Type
#define ieq_createQ(thrd, type, name, options, ppolicyInfo, hstoreObj, hstoreProps, pqhdl) \
  QInterface[(type) == intermediate ? multiConsumer : (type)].createQ(thrd, name, options, ppolicyInfo, hstoreObj, hstoreProps, pqhdl)
#else
/// Macro used to invoke create for the supplied Queue Type
#define ieq_createQ(thrd, type, name, options, ppolicyInfo, hstoreObj, hstoreProps, resourceSet, pqhdl) \
  QInterface[type].createQ(thrd, name, options, ppolicyInfo, hstoreObj, hstoreProps, resourceSet, pqhdl)
#endif
/// Macro used to invoke the appropriate deleteQ for the supplied Queue Handle
#define ieq_delete(thrd, pqhdl, freeOnly) \
  ((ismEngine_Queue_t *)(*pqhdl))->pInterface->deleteQ(thrd, pqhdl, freeOnly)
/// Macro used to invoke the appropriate drainQ function for the supplied Queue Handle
#define ieq_drain(thrd, qhdl) \
  ((ismEngine_Queue_t *)qhdl)->pInterface->drainQ(thrd, qhdl)
/// Macro used to invoke the appropriate dumpQ function for the supplied Queue Handle
#define ieq_dump(thrd, qhdl, dumpHdl) \
  ((ismEngine_Queue_t *)qhdl)->pInterface->dumpQ(thrd, qhdl, dumpHdl)
/// Macro used to invoke the appropriate getStats function for the supplied Queue Handle
#define ieq_getStats(thrd, qhdl, stats) \
  ((ismEngine_Queue_t *)qhdl)->pInterface->getStats(thrd, qhdl, stats)
/// Macro used to invoke the appropriate setPutsAttemptedStat function for the supplied Queue Handle
#define ieq_setStats(qhdl, stats, setType) \
  ((ismEngine_Queue_t *)qhdl)->pInterface->setStats(qhdl, stats, setType)
/// Macro used to invoke the appropriate checkAvailableMsgs function for the supplied Queue Handle
#define ieq_checkAvailableMsgs(qhdl, consumer) \
  ((ismEngine_Queue_t *)qhdl)->pInterface->checkAvailableMsgs(qhdl, consumer)
///Calls the correct checkWaiters function for the given queue
#define ieq_checkWaiters(thrd,qhdl,asyncdata) \
  ((ismEngine_Queue_t *)qhdl)->pInterface->checkWaiters(thrd, qhdl,asyncdata)
/// Macro used to invoke the appropriate put function for the supplied Queue Handle
#define ieq_put(thrd, qhdl, options, tran, msg, mtype) \
  ((ismEngine_Queue_t *)qhdl)->pInterface->put(thrd, qhdl, options, tran, msg, mtype)
/// Macro used to invoke the appropriate rehydrate function for the supplied Queue Handle
#define ieq_rehydrateMsg(thrd, qhdl, msg, tran, hdl, ref) \
  ((ismEngine_Queue_t *)qhdl)->pInterface->rehydrateMsg(thrd, qhdl, msg, tran, hdl, ref)
/// Macro used to invoke the appropriate rehydrate delivery id function for the supplied Queue Handle
#define ieq_rehydrateDeliveryId(thrd, qhdl, hMsgDelInfo, hdl, deliveryId, ppnode) \
  ((ismEngine_Queue_t *)qhdl)->pInterface->rehydrateDeliveryId(thrd, qhdl, hMsgDelInfo, hdl, deliveryId, ppnode)
/// Macro used to invole the appropriate mark message function for the supplied queue handle
#define ieq_markMessageGotInTran(thrd, qhdl, qOid, pTranData) \
  ((ismEngine_Queue_t *)qhdl)->pInterface->markMsgTranGot(thrd, qhdl, qOid, pTranData)
/// Macro used to invoke the appropriate initWaiter function for the supplied Queue Handle
#define ieq_initWaiter(thrd, qhdl, consumer) \
  ((ismEngine_Queue_t *)qhdl)->pInterface->initWaiter(thrd, qhdl, consumer)
/// Macro used to invoke the appropriate termWaiter function for the supplied Queue Handle
#define ieq_termWaiter(thrd, qhdl, consumer) \
  ((ismEngine_Queue_t *)qhdl)->pInterface->termWaiter(thrd, qhdl, consumer)
/// Macro used to invoke the appropriate acknowledge function for the supplied Queue Handle
#define ieq_acknowledge(thrd, qhdl, session, tran, pdlv, options, asyncdata) \
  ((ismEngine_Queue_t *)qhdl)->pInterface->acknowledge(thrd, qhdl, session, tran, pdlv, options, asyncdata)
/// Macro used to invoke the appropriate acknowledge function for the supplied Queue Handle
#define ieq_relinquish(thrd, qhdl, pdlv, relinqtype, pstoreopcnt) \
  ((ismEngine_Queue_t *)qhdl)->pInterface->relinquish(thrd, qhdl, pdlv, relinqtype, pstoreopcnt)
/// Macro used to invoke the appropriate prepareAck (ack phase 1) function for the supplied Queue Handle
#define ieq_prepareAck(thrd, qhdl, session, tran, pdlv, options, storeops) \
  ((ismEngine_Queue_t *)qhdl)->pInterface->prepareAck(thrd, qhdl, session, tran, pdlv, options, storeops)
/// Macro used to invoke the appropriate processAck (ack phase 2) function for the supplied Queue Handle
#define ieq_processAck(thrd, qhdl, session, tran, pdlv, options, phMsgToUnstore, triggerRedeliv, state, ppJobThrd) \
  ((ismEngine_Queue_t *)qhdl)->pInterface->processAck(thrd, qhdl, session, tran, pdlv, options, phMsgToUnstore, triggerRedeliv, state, ppJobThrd)
/// Macro used to invoke the appropriate completeAckBatch function for the supplied Queue Handle
#define ieq_completeAckBatch(thrd, qhdl, session, state) \
  ((ismEngine_Queue_t *)qhdl)->pInterface->completeAckBatch(thrd, qhdl, session, state)
/// Macro used to invoke the appropriate getDefnHdl function for the supplied Queue Handle
#define ieq_getDefnHdl(qhdl) \
  ((ismEngine_Queue_t *)qhdl)->pInterface->getDefnHdl(qhdl)
/// Macro used to invoke the appropriate getPropsHdl function for the supplied Queue Handle
#define ieq_getPropsHdl(qhdl) \
  ((ismEngine_Queue_t *)qhdl)->pInterface->getPropsHdl(qhdl)
/// Macro used to invoke the appropriate setPropsHdl function for the supplied Queue Handle
#define ieq_setPropsHdl(qhdl, propsHdl) \
  ((ismEngine_Queue_t *)qhdl)->pInterface->setPropsHdl(qhdl, propsHdl)
/// Macro used to invoke the appropriate updateProperties for the supplied Queue Handle
#define ieq_updateProperties(thrd, qhdl, qname, qopts, propshdl, resourceSet) \
  ((ismEngine_Queue_t *)qhdl)->pInterface->updateProperties(thrd, qhdl, qname, qopts, propshdl, resourceSet)
/// Macro used to invoke the appropriate completeRehydrate function for the supplied Queue Handle
#define ieq_completeRehydrate(thrd, qhdl) \
  ((ismEngine_Queue_t *)qhdl)->pInterface->completeRehydrate(thrd, qhdl)
/// Macro used to invoke the appropriate markQDeleted function for the supplied Queue Handle
#define ieq_markQDeleted(thrd, qhdl, updateStore) \
    ((ismEngine_Queue_t *)qhdl)->pInterface->markQDeleted(thrd, qhdl, updateStore)
/// Macro used to invoke the appropriate isDeleted function for the supplied Queue Handle
#define ieq_isDeleted(qhdl) \
    ((ismEngine_Queue_t *)qhdl)->pInterface->isDeleted(qhdl)
/// Macro used to invoke the appropriate reducePreDeleteCount function for the supplied Queue Handle
#define ieq_reducePreDeleteCount(thrd, qhdl) \
    ((ismEngine_Queue_t *)qhdl)->pInterface->reduceCount(thrd, qhdl)
/// Macro used to get the queue type out of the queue handle
#define ieq_getQType(qhdl) \
  (((ismEngine_Queue_t *)qhdl)->QType)
// Macro used to get the policy information (pointer) from the queue handle
#define ieq_getPolicyInfo(qhdl) \
  (((ismEngine_Queue_t *)qhdl)->PolicyInfo)
// Macro used to forget any tracking of inflight messages the queue is doing (MQTT-specific)
#define ieq_forgetInflightMsgs(thrd, qhdl) \
  (((ismEngine_Queue_t *)qhdl)->pInterface->forgetInflightMsgs(thrd, qhdl))
// Macro used to mark a queue as only delivering inflight messages that the delivery
//must complete for even after an unsub/reconnect (MQTT-specific)
#define ieq_redeliverEssentialOnly(thrd, qhdl) \
  (((ismEngine_Queue_t *)qhdl)->pInterface->redeliverOnly(thrd, qhdl))
// Macro used to clear some old messages off a queue and make space for new ones
#define ieq_reclaimSpace(thrd, qhdl, takeLock) \
  (((ismEngine_Queue_t *)qhdl)->pInterface->reclaimSpace(thrd, qhdl, takeLock))
// Macro used to remove messages from MDR table and queue for a client
#define ieq_relinquishAllMsgsForClient(thrd, qhdl, hmdi, relinqtype) \
  (((ismEngine_Queue_t *)qhdl)->pInterface->relinquishAll(thrd, qhdl, hmdi, relinqtype))
// Macro used to remove unneeded queues post-rehydration
#define ieq_removeIfUnneeded(thrd, qhdl) \
  (((ismEngine_Queue_t *)qhdl)->pInterface->removeIfUnneeded(thrd, qhdl))
// Macro used to remove expired msgs from queue
#define ieq_reapExpiredMsgs(thrd, qhdl, nowexpire, forcefullscan, expiryListLocked) \
  (((ismEngine_Queue_t *)qhdl)->pInterface->reapExpiredMsgs(thrd, qhdl, nowexpire, forcefullscan, expiryListLocked))
// Macro used to record a messageExpired
#define ieq_messageExpired(thrd, qhdl) \
  (((ismEngine_Queue_t *)qhdl)->pInterface->messageExpired(thrd, qhdl))
// Macro used to export buffered messages from a queue
//             (not messages in a transaction and from multiconsumerQ: not inflight messages)
#define ieq_exportMessages(thrd, qhdl, control) \
  (((ismEngine_Queue_t *)qhdl)->pInterface->exportMessages(thrd, qhdl, control))
// Macro used to finish the importing of a queue
#define ieq_completeImport(thrd, qhdl) \
  (((ismEngine_Queue_t *)qhdl)->pInterface->completeImport(thrd, qhdl))
// Macro used to get stats for consumer on a queue
#define ieq_getConsumerStats(thrd, qhdl, memPoolHdl,pNumConsumers,consDataArray) \
  (((ismEngine_Queue_t *)qhdl)->pInterface->getConsumerStats(thrd, qhdl, memPoolHdl,pNumConsumers,consDataArray ))

int32_t ieq_rehydrateQ(ieutThreadData_t *pThreadData,
                       ismStore_Handle_t recHandle,
                       ismStore_Record_t *record,
                       ismEngine_RestartTransactionData_t *transData,
                       void **outData,
                       void *pContext);

int32_t ieq_rehydrateQueueProps(ieutThreadData_t *pThreadData,
                                ismStore_Handle_t recHandle,
                                ismStore_Record_t *record,
                                ismEngine_RestartTransactionData_t *transData,
                                void *requestingRecord,
                                void **rehydratedRecord,
                                void *pContext);

int32_t ieq_rehydrateQueueMsgRef(ieutThreadData_t *pThreadData,
                                 void *owner,
                                 void *child,
                                 ismStore_Handle_t refHandle,
                                 ismStore_Reference_t *reference,
                                 ismEngine_RestartTransactionData_t *transData,
                                 void *pContext);

bool ieq_setPolicyInfo(ieutThreadData_t *pThreadData,
                       ismEngine_Queue_t *pQueue,
                       iepiPolicyInfo_t *pPolicyInfo);

//Remove any qnodes we rehydrated but which came back consumed...
int32_t ieq_removeRehydratedConsumedNodes(ieutThreadData_t *pThreadData);

void ieq_scheduleCheckWaiters(ieutThreadData_t *pThreadData,
                              ismEngine_Queue_t *Q);

/// Array of ieqInterface_t structures containing function pointers to the various implemnattions of the QInterface
extern const ieqInterface_t QInterface[];
///  @}


//How many messages should remain on a subscription after reclaiming space from old messages (to make way for new ones)
//(Expressed as a fraction of the max messages count for that subscription
#define IEQ_RECLAIMSPACE_MSGS_SURVIVE_FRACTION 0.95
//How many message bytes should remain on a remote server after reclaiming space from old messages
#define IEQ_RECLAIMSPACE_MSGBYTES_SURVIVE_FRACTION 0.95


#endif
