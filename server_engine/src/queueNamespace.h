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

//****************************************************************************
/// @file  queueNamespace.h
/// @brief Engine component queue namespace (named queues) header file.
//****************************************************************************
#ifndef __ISM_QUEUENAMESPACE_DEFINED
#define __ISM_QUEUENAMESPACE_DEFINED

#include "queueCommon.h"        // ieq functions & constants

typedef enum tag_ieqnDiscardMsgsOpt_t {
    ieqnDiscardMessages = true,
    ieqnKeepMessages = false
} ieqnDiscardMsgsOpt_t;

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Structure used to maintain a namespace of queues
///  @remark
///    This structure is used as the anchor for the list of queue names.
///////////////////////////////////////////////////////////////////////////////
typedef struct tag_ieqnQueueNamespace_t
{
    char                   strucId[4];   ///< Structure identifier 'EQNS'
    ieutHashTableHandle_t  names;        ///< Queue handles by queue name
    pthread_rwlock_t       namesLock;    ///< Lock on the queue names
} ieqnQueueNamespace_t;

#define ieqnQUEUENAMESPACE_STRUCID "EQNS"

typedef struct tag_ieqnQueue_t
{
  char                     strucId[4];           ///< Strucid 'EQNQ'
  uint32_t                 useCount;             ///< How many producers & consumers exist for the queue
  uint32_t                 consumerCount;        ///< Specifically how many consumers exist for the queue
  uint32_t                 producerCount;        ///< Specifically how many producers exist for the queue
  ismQHandle_t             queueHandle;          ///< Handle of the actual queue
  char                    *queueName;            ///< Name of this queue
  ismEngine_ClientState_t *creator;              ///< Client state of the creator (temporary queues)
  struct tag_ieqnQueue_t  *nextInGroup;          ///< Next queue in group (temporary queues)
  bool                     temporary;            ///< Whether this is a temporary queue or not
} ieqnQueue_t;

#define ieqnQUEUE_STRUCID "EQNQ"

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_ieqnQueue_t(__file)\
    iedm_descriptionStart(__file, ieqnQueue_t, strucId, ieqnQUEUE_STRUCID);\
    iedm_describeMember(char [4],                   strucId);\
    iedm_describeMember(uint32_t,                   useCount);\
    iedm_describeMember(uint32_t,                   consumerCount);\
    iedm_describeMember(uint32_t,                   producerCount);\
    iedm_describeMember(ismQHandle_t,               queueHandle);\
    iedm_describeMember(char *,                     queueName);\
    iedm_describeMember(ismEngine_ClientState_t *,  creator);\
    iedm_describeMember(ieqnQueue_t *,              nextInGroup);\
    iedm_describeMember(bool,                       temporary);\
    iedm_descriptionEnd;

#define ieqnINITIAL_QUEUENAMESPACE_CAPACITY 1000  ///< Initial capacity of queue names hash

int32_t ieqn_initEngineQueueNamespace(ieutThreadData_t *pThreadData);
ieqnQueueNamespace_t *ieqn_getEngineQueueNamespace(void);
void ieqn_destroyQueueNamespace(ieutThreadData_t *pThreadData,
                                ieqnQueueNamespace_t *queues);
void ieqn_destroyEngineQueueNamespace(ieutThreadData_t *pThreadData);
int32_t ieqn_reconcileEngineQueueNamespace(ieutThreadData_t *pThreadData);

int ieqn_queueConfigCallback(ieutThreadData_t *pThreadData,
                             char *objectIdentifier,
                             ism_prop_t *changedProps,
                             ism_ConfigChangeType_t changeType);

int32_t ieqn_addQueue(ieutThreadData_t *pThreadData,
                      const char *queueName,
                      ismQHandle_t queueHandle,
                      ieqnQueue_t **ppQueue);
int32_t ieqn_createQueue(ieutThreadData_t *pThreadData,
                         const char *pQueueName,
                         ismQueueType_t QueueType,
                         ismQueueScope_t queueScope,
                         ismEngine_ClientState_t *pCreator,
                         ism_prop_t *pProperties,
                         const char *pPropQualifier,
                         ieqnQueueHandle_t *ppQueue);
int32_t ieqn_openQueue(ieutThreadData_t *pThreadData,
                       ismEngine_ClientState_t *pClient,
                       const char *pQueueName,
                       ismEngine_Consumer_t *pConsumer,
                       ismEngine_Producer_t *pProducer);
void ieqn_dumpWriteDescriptions(iedmDumpHandle_t dumpHdl);
void ieqn_dumpQueue(ieutThreadData_t *pThreadData,
                    ieqnQueue_t *pQueue,
                    iedmDumpHandle_t dumpHdl);
int32_t ieqn_dumpQueueByName(ieutThreadData_t *pThreadData,
                             const char *queueName,
                             iedmDumpHandle_t dumpHdl);
int32_t ieqn_destroyQueue(ieutThreadData_t *pThreadData,
                          const char *pQueueName,
                          ieqnDiscardMsgsOpt_t discardMsgsOpt,
                          bool adminRequest);
void ieqn_addQueueToGroup(ieutThreadData_t *pThreadData,
                          ieqnQueue_t *pQueue,
                          ieqnQueue_t **ppQueueGroup);
int32_t ieqn_removeQueueFromGroup(ieutThreadData_t *pThreadData,
                                  const char *pQueueName,
                                  ieqnQueue_t **ppQueueGroup);
int32_t ieqn_destroyQueueGroup(ieutThreadData_t *pThreadData,
                               ieqnQueue_t *pQueueGroup,
                               ieqnDiscardMsgsOpt_t discardMsgsOpt);
int32_t ieqn_isTemporaryQueue(const char *pQueueName,
                              bool *pTemporary,
                              ismEngine_ClientState_t **ppCreator);
ismQHandle_t ieqn_getQueueHandle(ieutThreadData_t *pThreadData,
                                 const char *pQueueName);
int32_t ieqn_getQueueStats(ieutThreadData_t *pThreadData,
                           const char *pQueueName,
                           ismEngine_QueueStatistics_t *pQStats);
void ieqn_unregisterConsumer(ieutThreadData_t *pThreadData,
                             ismEngine_Consumer_t *consumer);
void ieqn_unregisterProducer(ieutThreadData_t *pThreadData,
                             ismEngine_Producer_t *producer);

#endif /* __ISM_QUEUENAMESPACE_DEFINED */

/*********************************************************************/
/* End of engineStore.h                                              */
/*********************************************************************/
