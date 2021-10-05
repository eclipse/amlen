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
/// @file  queueNamespace.c
/// @brief Engine component queue namespace functions
//****************************************************************************
#define TRACE_COMP Engine

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <assert.h>

#include "engineInternal.h"
#include "msgCommon.h"          // iem functions & constants
#include "engineStore.h"        // iest functions & constants
#include "queueNamespace.h"     // ieqn functions & constants
#include "engineHashTable.h"    // Hash table functions & constants
#include "engineDump.h"         // iedm functions & constants

// local prototypes
static void ieqn_releaseQueue(ieutThreadData_t *pThreadData,
                              ieqnQueue_t *queue);
static void ieqn_registerConsumer(ieutThreadData_t *pThreadData,
                                  ieqnQueue_t *queue,
                                  ismEngine_Consumer_t *consumer);
static void ieqn_registerProducer(ieutThreadData_t *pThreadData,
                                  ieqnQueue_t *queue,
                                  ismEngine_Producer_t *producer);
static uint32_t ieqn_generateQueueNameHash(const char *key);

//****************************************************************************
/// @brief Create and initialize the global engine queue namespace
///
/// @remark Will replace the existing global queue namespace without checking
///         that this has happened.
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @see ieqn_destroyEngineQueueNamespace
//****************************************************************************
int32_t ieqn_initEngineQueueNamespace(ieutThreadData_t *pThreadData)
{
    int osrc = 0;
    int32_t rc = OK;
    ieqnQueueNamespace_t *queues = NULL;
    pthread_rwlockattr_t rwlockattr_init;

    ieutTRACEL(pThreadData, 0, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    osrc = pthread_rwlockattr_init(&rwlockattr_init);

    if (osrc) goto mod_exit;

    osrc = pthread_rwlockattr_setkind_np(&rwlockattr_init, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);

    if (osrc) goto mod_exit;

    // Create the namespace structure
    queues = iemem_calloc(pThreadData, IEMEM_PROBE(iemem_queueNamespace, 1), 1, sizeof(ieqnQueueNamespace_t));

    if (NULL == queues)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    memcpy(queues->strucId, ieqnQUEUENAMESPACE_STRUCID, 4);

    osrc = pthread_rwlock_init(&queues->namesLock, &rwlockattr_init);

    if (osrc) goto mod_exit;

    rc = ieut_createHashTable(pThreadData,
                              ieqnINITIAL_QUEUENAMESPACE_CAPACITY,
                              iemem_queueNamespace,
                              &(queues->names));

    if (rc != OK)
    {
        (void)pthread_rwlock_destroy(&queues->namesLock);
        goto mod_exit;

    }

mod_exit:

    if (osrc)
    {
        rc = ISMRC_Error;
        ism_common_setError(rc);
    }

    if (rc != OK)
    {
        ieqn_destroyQueueNamespace(pThreadData, queues);
    }
    else
    {
        assert(queues != NULL);
        ismEngine_serverGlobal.queues = queues;
    }

    ieutTRACEL(pThreadData, queues,  ENGINE_FNC_TRACE, FUNCTION_EXIT "queues=%p, rc=%d\n", __func__, queues, rc);

    return rc;
}

//****************************************************************************
/// @brief Returns the global engine queue namespace to the caller
///
/// @return Pointer to queue namespace, NULL if not initialized.
///
/// @see ieqn_initEngineQueueNamespace
/// @see ieqn_destroyEngineQueueNamespace
//****************************************************************************
ieqnQueueNamespace_t *ieqn_getEngineQueueNamespace(void)
{
    return ismEngine_serverGlobal.queues;
}

//****************************************************************************
/// @brief Clean up / free the content of the queue namespace
///
/// @param[in]     key      Key of the hash table entry being passed
/// @param[in]     keyHash  Hash of the key
/// @param[in]     value    Value of the hash entry (pointer to named queue)
/// @param[in]     context  The context passed to the callback routine
///
/// We use this to avoid lots of additional levels when recursing, when first
/// called the key and keyHash are not valid.
//****************************************************************************
void ieqn_destroyQueueNamespaceCallback(ieutThreadData_t *pThreadData,
                                        char *key,
                                        uint32_t keyHash,
                                        void *value,
                                        void *context)
{
    ieqnQueue_t *queue = (ieqnQueue_t *)value;

    (void)ieq_delete(pThreadData, &queue->queueHandle, true);

    iemem_freeStruct(pThreadData, iemem_queueNamespace, queue, queue->strucId);
}

//****************************************************************************
/// @brief Destroy and the specified queue namespace structure
///
/// @param[in]     queues  Pointer to the queue namespace header to destroy
//****************************************************************************
void ieqn_destroyQueueNamespace(ieutThreadData_t *pThreadData,
                                ieqnQueueNamespace_t *queues)
{
    if (NULL == queues) goto mod_exit;

    // NOTE: We do not get the lock (which we are going to destroy) since
    //       no-one else should be using it at the time we destroy it.

    if (NULL != queues->names)
    {
        ieut_traverseHashTable(pThreadData,
                               queues->names,
                               &ieqn_destroyQueueNamespaceCallback,
                               NULL);
        ieut_destroyHashTable(pThreadData, queues->names);

        (void)pthread_rwlock_destroy(&queues->namesLock);
    }

    iemem_freeStruct(pThreadData, iemem_queueNamespace, queues, queues->strucId);

mod_exit:

    return;
}

//****************************************************************************
/// @brief Destroy and remove the global engine queue namespace
///
/// @remark This will result in all queue information being discarded.
///
/// @see ieqn_getEngineQueueNamespace
/// @see ieqn_initEngineQueueNamespace
//****************************************************************************
void ieqn_destroyEngineQueueNamespace(ieutThreadData_t *pThreadData)
{
    ieutTRACEL(pThreadData, ismEngine_serverGlobal.queues, ENGINE_SHUTDOWN_DIAG_TRACE,
               FUNCTION_ENTRY "\n", __func__);

    ieqn_destroyQueueNamespace(pThreadData, ismEngine_serverGlobal.queues);

    ismEngine_serverGlobal.queues = NULL;

    ieutTRACEL(pThreadData, 0, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_EXIT "\n", __func__);
}

//****************************************************************************
/// @brief Build a list of the queue names using a specified policy info structure
///
/// @param[in]     key      Key of the hash table entry being passed
/// @param[in]     keyHash  Hash of the key
/// @param[in]     value    Value of the hash entry (pointer to named queue)
/// @param[in]     context  The context passed to the callback routine
//****************************************************************************
typedef struct tag_ieqnFindQueueByPolicyInfoCbContext_t
{
    char            **queueNames;  ///< Names of queues that are not reconciled
    int32_t           count;       ///< Count of unreconciled names
    int32_t           capacity;    ///< Capacity of the array
    iepiPolicyInfo_t *policyInfo;  ///< Policy Info structure to identify
} ieqnFindQueueByPolicyInfoCbContext_t;


void ieqn_findQueueByPolicyInfoCallback(ieutThreadData_t *pThreadData,
                                        char *key,
                                        uint32_t keyHash,
                                        void *value,
                                        void *context)
{
    ieqnFindQueueByPolicyInfoCbContext_t *pContext = (ieqnFindQueueByPolicyInfoCbContext_t *)context;

    ieqnQueue_t *namedQueue = (ieqnQueue_t *)value;

    // If this queue refers to the policy info we are interested in, add it to the list
    if (namedQueue->queueHandle->PolicyInfo == pContext->policyInfo)
    {
        if (pContext->count == pContext->capacity)
        {
            char **newNames = iemem_realloc(pThreadData,
                                            IEMEM_PROBE(iemem_queueNamespace, 2),
                                            pContext->queueNames,
                                            (pContext->capacity+10)*sizeof(char *));

            if (newNames != NULL)
            {
                pContext->queueNames = newNames;
                pContext->capacity += 10;
            }
        }

        if (pContext->count < pContext->capacity)
        {
            pContext->queueNames[pContext->count] = iemem_malloc(pThreadData,
                                                                 IEMEM_PROBE(iemem_queueNamespace, 3),
                                                                 strlen(key)+1);

            if (NULL != pContext->queueNames[pContext->count])
            {
                strcpy(pContext->queueNames[pContext->count], key);
                pContext->count+=1;
            }
        }
    }
}

//****************************************************************************
/// @brief Reconcile the contents of the engine queue namespace with the admin
///        component's queue definitions.
///
/// @remark This function does not take locks, it is expected to be called
///         during restart / recovery and so is expecting no contention.
//****************************************************************************
int32_t ieqn_reconcileEngineQueueNamespace(ieutThreadData_t *pThreadData)
{
    int32_t rc = OK;
    ieqnQueueNamespace_t *queues = ismEngine_serverGlobal.queues;
    iepiPolicyInfo_t *defaultPolicyInfo = iepi_getDefaultPolicyInfo(false);

    ieutTRACEL(pThreadData, defaultPolicyInfo, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Get Queue object names from the configuration
    ism_prop_t *queueNames = ism_config_getObjectInstanceNames(ismEngine_serverGlobal.configCallbackHandle,
                                                               ismENGINE_ADMIN_VALUE_QUEUE);

    // Loop through the queue names in the configuration ensuring we have an up-to-date
    // view of each.
    if (queueNames != NULL)
    {
        const char *thisQueueName = NULL;

        for (int32_t i = 0; ism_common_getPropertyIndex(queueNames, i, &thisQueueName) == 0; i++)
        {
            // This was not a queue that we had already found from store records, yet
            // the configuration had a definition for it - this is unexpected, but might
            // be caused by the configuration having been modified while the server
            // was off-line, legitimately.
            //
            // So we write out a trace line, but carry on creating the queue.
            if (ieqn_getQueueHandle(pThreadData, thisQueueName) == NULL)
            {
                ieutTRACEL(pThreadData, i, ENGINE_INTERESTING_TRACE,
                           "Creating unrecognised queue '%s' from configuration.\n",
                           thisQueueName);
            }

            // Get Queue configuration information for this queue
            ism_prop_t *queueProps = ism_config_getProperties(ismEngine_serverGlobal.configCallbackHandle,
                                                              ismENGINE_ADMIN_VALUE_QUEUE,
                                                              thisQueueName);

            if (queueProps != NULL)
            {
                // Create the queue - this will _update_ the Policy info for an existing queue.
                rc = ieqn_createQueue(pThreadData,
                                      thisQueueName,
                                      multiConsumer,
                                      ismQueueScope_Server, NULL,
                                      queueProps,
                                      thisQueueName,
                                      NULL);

                ism_common_freeProperties(queueProps);
            }
        }

        ism_common_freeProperties(queueNames);
    }

    ieqnFindQueueByPolicyInfoCbContext_t context = {NULL, 0, 0, defaultPolicyInfo};

    // Now look through the hash table for queues which still refer to the default
    // policy info - these did not appear in the config properties, i.e. are unreconciled.
    ieut_traverseHashTable(pThreadData,
                           queues->names,
                           &ieqn_findQueueByPolicyInfoCallback,
                           &context);

    // Any unreconciled queues should be deleted now (unless auto-creation is enabled)
    for(int i=0; i<context.count; i++)
    {
        if (rc == OK)
        {
            // If auto creation is disabled, destroy any queues not reconciled with config
            if (ismEngine_serverGlobal.disableAutoQueueCreation == true)
            {
                ieutTRACEL(pThreadData, i, ENGINE_WORRYING_TRACE, "Destroying unreconciled queue '%s'\n", context.queueNames[i]);

                rc = ieqn_destroyQueue(pThreadData, context.queueNames[i], ieqnDiscardMessages, false);

                if (rc != OK)
                {
                    ism_common_setError(rc);
                }
                else
                {
                    LOG(INFO, Messaging, 3008, "%-s",
                        "Queue {0} was deleted because it is no longer defined in the server configuration.",
                        context.queueNames[i]);
                }
            }
            // If auto creation is enabled, assume the queue was auto-created
            else
            {
                ieutTRACEL(pThreadData, i, ENGINE_NORMAL_TRACE, "Unreconciled queue '%s', assuming auto-created.\n", context.queueNames[i]);
            }
        }

        iemem_free(pThreadData, iemem_queueNamespace, context.queueNames[i]);
    }

    if (NULL != context.queueNames) iemem_free(pThreadData, iemem_queueNamespace, context.queueNames);

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief  Add the specified queue to the namespace with the specified name
///
/// Adds a name to the queue namespace handle
///
/// @param[in]     queueName    Name of the queue
/// @param[in]     queueHandle  Handle of the (internal) queue
/// @param[out]    ppQueue      (optional) pointer receive a pointer to the
///                             created queue.
///
/// @remarks The lock for the queue namespace must be held for WRITE, other
///          than in recovery where no locking is used.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ieqn_addQueue(ieutThreadData_t *pThreadData,
                      const char *queueName,
                      ismQHandle_t queueHandle,
                      ieqnQueue_t **ppQueue)
{
    int32_t rc;
    ieqnQueue_t *queue = NULL;

    ieutTRACEL(pThreadData, queueHandle, ENGINE_FNC_TRACE, FUNCTION_ENTRY "queueName='%s' queueHandle=%p\n",
               __func__, queueName, queueHandle);

    queue = iemem_calloc(pThreadData,
                         IEMEM_PROBE(iemem_queueNamespace, 4), 1,
                         sizeof(ieqnQueue_t)+strlen(queueName)+1);

    if (queue == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    ismEngine_SetStructId(queue->strucId, ieqnQUEUE_STRUCID);
    queue->queueHandle = queueHandle;
    queue->queueName = (char *)(queue+1);
    queue->useCount = 1; // The reference in the hash table counts as a 'user'

    strcpy(queue->queueName, queueName);

    ieqnQueueNamespace_t *queues = ismEngine_serverGlobal.queues;
    uint32_t queueNameHash = ieqn_generateQueueNameHash(queueName);

    rc = ieut_putHashEntry(pThreadData,
                           queues->names,
                           ieutFLAG_DUPLICATE_NONE,
                           queue->queueName,
                           queueNameHash,
                           queue,
                           0);

    if (rc != OK) goto mod_exit;

    if (NULL != ppQueue) *ppQueue = queue;

mod_exit:

    if (rc != OK && NULL != queue)
    {
        iemem_freeStruct(pThreadData, iemem_queueNamespace, queue, queue->strucId);
    }

    ieutTRACEL(pThreadData, queue,  ENGINE_FNC_TRACE, FUNCTION_EXIT "queue=%p, rc=%d\n", __func__, queue, rc);

    return rc;
}

//****************************************************************************
/// @brief  Internal function to create a named queue
///
/// Creates a queue with a given name and link the specified consumer to it.
///
/// @param[in]     pQueueName         Name of the queue
/// @param[in]     QueueType          Type of queue to create
/// @param[in]     QueueScope         Whether this is a temporary or permanent queue
/// @param[in]     pCreator           Creating client (only required for temporary queues)
/// @param[in]     pProperties        Queue properties.
/// @param[in]     pPropQualifier     String qualifying properties in pProperties
/// @param[out]    ppQueue            Optional pointer to receive the queue pointer for
///                                   a newly created queue.
///
/// @remark If the specified queue already exists the the queue is updated, but the
///         pointer to the queue is not returned.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ieqn_createQueue(ieutThreadData_t *pThreadData,
                         const char *pQueueName,
                         ismQueueType_t QueueType,
                         ismQueueScope_t QueueScope,
                         ismEngine_ClientState_t *pCreator,
                         ism_prop_t *pProperties,
                         const char *pPropQualifier,
                         ieqnQueue_t **ppQueue)
{
    int32_t rc = OK;
    ieqnQueue_t *queue = NULL;
    bool namesLocked = false;
    ismQHandle_t queueHandle = NULL;
    char *propertyName = NULL;
    bool newQueue = true;
    iepiPolicyInfo_t *defaultPolicyInfo = iepi_getDefaultPolicyInfo(false);
    iepiPolicyInfo_t *tempPolicyInfo = defaultPolicyInfo;
    size_t propQualifierLen = pPropQualifier == NULL ? 0 : strlen(pPropQualifier);

    ieutTRACEL(pThreadData, pQueueName,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "pQueueName='%s'\n", __func__, pQueueName);

    assert(pQueueName != NULL);
    assert(QueueScope == ismQueueScope_Server || pCreator != NULL);

    ieqnQueueNamespace_t *queues = ismEngine_serverGlobal.queues;

    // Allocate a buffer to be used to get the concurrentConsumers value and to provide
    // a property format string - we can do both with a single allocation.
    assert(strlen(ismENGINE_ADMIN_QUEUE_PROPERTY_FORMAT) <
           strlen(ismENGINE_ADMIN_PREFIX_QUEUE_CONCURRENTCONSUMERS));

    propertyName = iemem_malloc(pThreadData,
                                IEMEM_PROBE(iemem_queueNamespace, 5),
                                strlen(ismENGINE_ADMIN_PREFIX_QUEUE_CONCURRENTCONSUMERS) + propQualifierLen  + 1);

    if (NULL == propertyName)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    uint32_t queueNameHash = ieqn_generateQueueNameHash(pQueueName);

    // Lock the names for write
    retry_get_queue:

    ismEngine_getRWLockForWrite(&queues->namesLock);
    namesLocked = true;

    // Check for the queue, in case someone else created it in the meantime
    rc = ieut_getHashEntry(queues->names,
                           pQueueName,
                           queueNameHash,
                           (void **)&queue);

    // If the queue did not already exist, create a queue of the requested
    // type.
    if (rc == ISMRC_NotFound)
    {
        ieqOptions_t queueOptions;

        if (QueueScope == ismQueueScope_Client)
        {
            queueOptions = ieqOptions_TEMPORARY_QUEUE;
        }
        else
        {
            queueOptions = ieqOptions_DEFAULT;
        }

        // Creation of a queue will write to the store, we don't want to be holding
        // a lock while writing to the store, so release it now - we will regain it
        // before leaving this function, so don't bother modifying the boolean.
        ismEngine_unlockRWLock(&queues->namesLock);

        // Actually create the queue of requested type
        rc = ieq_createQ( pThreadData
                        , QueueType
                        , pQueueName
                        , queueOptions
                        , defaultPolicyInfo // Initialise with default
                        , ismSTORE_NULL_HANDLE
                        , ismSTORE_NULL_HANDLE
                        , iereNO_RESOURCE_SET
                        , &queueHandle );

        // Regain the lock, and check that we haven't been beaten to creation.
        ismEngine_getRWLockForWrite(&queues->namesLock);

        if (rc == OK)
        {
            // Get a reference on the policy we've assigned (the default)
            iepi_acquirePolicyInfoReference(defaultPolicyInfo);

            rc = ieut_getHashEntry(queues->names,
                                   pQueueName,
                                   queueNameHash,
                                   (void **)&queue);

            // We have been beaten to it, delete the queue and start over
            if (rc == OK)
            {
                ismEngine_unlockRWLock(&queues->namesLock);

                (void)ieq_delete(pThreadData, &queueHandle, false);
                queueHandle = NULL;

                goto retry_get_queue;
            }

            // If we get here, we want the rest of the code to treat this as OK.
            rc = OK;
        }
    }
    else
    {
        assert(rc == OK);

        newQueue = false;
        queueHandle = queue->queueHandle;
        tempPolicyInfo = queueHandle->PolicyInfo;

        if (queue->temporary)
        {
            // Only allow temporary queues to be modified by the creator
            if (queue->creator != NULL && queue->creator != pCreator)
            {
                rc = ISMRC_DestNotValid;
                ism_common_setErrorData(rc, "%s", pQueueName);
                goto mod_exit;
            }

            assert(QueueScope == ismQueueScope_Client);

            // Expect basic property format
            strcpy(propertyName, ismENGINE_ADMIN_PROPERTY_CONCURRENTCONSUMERS);
        }
        else
        {
            // Expect admin property format
            strcpy(propertyName, ismENGINE_ADMIN_PREFIX_QUEUE_CONCURRENTCONSUMERS);
            if (propQualifierLen != 0)
            {
                strcat(propertyName, pPropQualifier);
            }
        }

        // Get the new value of concurrentConsumers
        bool newConcurrentConsumers = ism_common_getBooleanProperty(pProperties,
                                                                    propertyName,
                                                                    tempPolicyInfo->concurrentConsumers);

        // Fail if attempting to set concurrentConsumers to false with >1 consumer
        if (newConcurrentConsumers == false && queue->consumerCount > 1)
        {
            rc = ISMRC_TooManyConsumers;
            goto mod_exit;
        }
    }

    if (rc != OK) goto mod_exit;

    // Temporary queues do not use a property name format
    if (QueueScope == ismQueueScope_Client)
    {
        iemem_free(pThreadData, iemem_queueNamespace, propertyName);
        propertyName = NULL;
    }
    else
    {
        strcpy(propertyName, ismENGINE_ADMIN_QUEUE_PROPERTY_FORMAT);
        if (propQualifierLen != 0)
        {
            strcat(propertyName, pPropQualifier);
        }
    }

    // Create or update the policy info.
    if (tempPolicyInfo == defaultPolicyInfo)
    {
        // We create a private messaging policy from the queue's properties, the name
        // in these properties is the name of the queue and so we tell the create function
        // not to use it for the created policy.
        rc = iepi_createPolicyInfoFromProperties(pThreadData,
                                                 propertyName, // Format string
                                                 pProperties,
                                                 ismSEC_POLICY_LAST,
                                                 false,
                                                 false,
                                                 &tempPolicyInfo);
    }
    else
    {
        rc = iepi_updatePolicyInfoFromProperties(pThreadData,
                                                 tempPolicyInfo,
                                                 propertyName, // Format string
                                                 pProperties,
                                                 NULL);
    }

    if (rc != OK) goto mod_exit;

    // For a new queue, add to the namespace - we do this last
    if (newQueue)
    {
        rc = ieqn_addQueue(pThreadData,
                           pQueueName,
                           queueHandle,
                           &queue);

        if (rc != OK) goto mod_exit;

        DEBUG_ONLY bool changedPolicy = ieq_setPolicyInfo(pThreadData, queueHandle, tempPolicyInfo);
        assert(changedPolicy == true);

        // Remember whether this was a temporary queue or not
        queue->temporary = (QueueScope == ismQueueScope_Client);
        queue->creator = pCreator;
    }
    // Existing queues, update the policy info if using the default
    else if (queueHandle->PolicyInfo == defaultPolicyInfo)
    {
        DEBUG_ONLY bool changedPolicy = ieq_setPolicyInfo(pThreadData, queueHandle, tempPolicyInfo);
        assert(changedPolicy == true);
    }

mod_exit:

    if (rc == OK)
    {
        assert(queue != NULL);
        assert(queue->queueHandle != NULL);

        // For modified queues, or if not requested, we don't pass the queue back
        if (newQueue && NULL != ppQueue) *ppQueue = queue;
    }
    else if (newQueue)
    {
        if (queueHandle != NULL)
        {
            (void)ieq_delete(pThreadData, &queueHandle, false);
            queueHandle = NULL;
        }
        else if (tempPolicyInfo != defaultPolicyInfo)
        {
            // Release our use of the messaging policy info
            iepi_releasePolicyInfo(pThreadData, tempPolicyInfo);
        }

        // Since the last thing we do is allocate the queue, we should never
        // end up with a queue here - if we do, then it needs to be removed
        // from the hash table.
        assert(queue == NULL);
    }

    if (namesLocked) ismEngine_unlockRWLock(&queues->namesLock);

    if (NULL != propertyName) iemem_free(pThreadData, iemem_queueNamespace, propertyName);

    ieutTRACEL(pThreadData, queue, ENGINE_FNC_TRACE, FUNCTION_EXIT "queue=%p, queueHandle=%p, rc=%d\n", __func__,
               queue, queueHandle, rc);

    return rc;
}

//****************************************************************************
/// @brief  Internal function to open a named queue
///
/// Opens a queue with a given name and link the specified producer or consumer
/// to it.
///
/// @param[in]     pClient            (optional) Client state of opener
/// @param[in]     pQueueName         Subscription name
/// @param[in]     pConsumer          Consumer to link to queue
/// @param[in]     pProducer          Producer to link to the queue
///
/// @remarks Only one of pConsumer or pProducer must be set.
///
/// @remarks If the queue does not exist, the ability to automatically create the
///          queue is controlled by an engine property.
///
/// @return OK on successful completion, ISMRC_DestNotValid if the queue was
///         not found (and not auto-created) or an ISMRC_ value.
//****************************************************************************
int32_t ieqn_openQueue(ieutThreadData_t *pThreadData,
                       ismEngine_ClientState_t *pClient,
                       const char *pQueueName,
                       ismEngine_Consumer_t *pConsumer,
                       ismEngine_Producer_t *pProducer)
{
    int32_t rc = OK;
    ieqnQueue_t *queue = NULL;
    bool namesLocked = false;
    bool autoCreated = false;

    ieutTRACEL(pThreadData, pClient, ENGINE_FNC_TRACE, FUNCTION_ENTRY "pClient=%p, pQueueName='%s'\n", __func__,
               pClient, pQueueName);

    assert(pQueueName != NULL);
    assert(pConsumer != NULL || pProducer != NULL);

    uint32_t queueNameHash = ieqn_generateQueueNameHash(pQueueName);

    ieqnQueueNamespace_t *queues = ismEngine_serverGlobal.queues;

    // Look up the queue, and if permitted create it
    while(rc == OK)
    {
        ismEngine_getRWLockForRead(&queues->namesLock);
        namesLocked = true;

        // Look for the requested queue in the hash table.
        if (ieut_getHashEntry(queues->names,
                              pQueueName,
                              queueNameHash,
                              (void **)&queue) == OK)
        {
            break;
        }

        // If automatic named queue creation is not disabled, create the queue.
        if (ismEngine_serverGlobal.disableAutoQueueCreation == false)
        {
            ismEngine_unlockRWLock(&queues->namesLock);
            namesLocked = false;

            rc = ieqn_createQueue(pThreadData,
                                  pQueueName,
                                  multiConsumer,
                                  ismQueueScope_Server, NULL,
                                  NULL, // No properties, use default
                                  NULL,
                                  NULL);

            if (rc != OK) break;

            autoCreated = true;
        }
        else
        {
            rc = ISMRC_DestNotValid;
            ism_common_setErrorData(rc, "%s", pQueueName);
        }
    }

    if (rc != OK) goto mod_exit;

mod_exit:

    if (rc == OK)
    {
        assert(namesLocked == true);
        assert(queue != NULL);
        assert(queue->queueHandle != NULL);

        if (NULL != pConsumer)
        {
            assert(pProducer == NULL);

            // Concurrent consumers are disallowed, and there are consumers.
            if (queue->queueHandle->PolicyInfo->concurrentConsumers == false && queue->consumerCount > 0)
            {
                rc = ISMRC_TooManyConsumers;
            }
            else
            {
                ieqn_registerConsumer(pThreadData, queue, pConsumer);
            }
        }
        else if (NULL != pProducer)
        {
            assert(pConsumer == NULL);

            if (queue->queueHandle->PolicyInfo->allowSend == false)
            {
                rc = ISMRC_SendNotAllowed;
            }
            else
            {
                ieqn_registerProducer(pThreadData, queue, pProducer);
            }
        }
    }

    if (namesLocked)
    {
        ismEngine_unlockRWLock(&queues->namesLock);
    }

    ieutTRACEL(pThreadData, autoCreated,  ENGINE_FNC_TRACE, FUNCTION_EXIT "autoCreated=%d, rc=%d\n", __func__, autoCreated, rc);

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief Dump the specified queue
///
/// @param[in]     queue     Queue to be dumped
/// @param[in]     dumpHdl   Dump to write to
//****************************************************************************
void ieqn_dumpQueue(ieutThreadData_t *pThreadData, ieqnQueue_t *queue, iedmDumpHandle_t dumpHdl)
{
    iedmDump_t *dump = (iedmDump_t *)dumpHdl;

    if (iedm_dumpStartObject(dump, queue) == true)
    {
        iedm_dumpStartGroup(dump, "Queue");

        iedm_dumpData(dump, "ieqnQueue_t", queue, iemem_usable_size(iemem_queueNamespace, queue));

        if (dump->detailLevel >= iedmDUMP_DETAIL_LEVEL_5)
        {
            ieq_dump(pThreadData, queue->queueHandle, dumpHdl);
        }

        iedm_dumpEndGroup(dump);
        iedm_dumpEndObject(dump, queue);
    }
}

//****************************************************************************
/// @internal
///
/// @brief Dump the specified named queue
///
/// @param[in]     queueName    Name of the queue
/// @param[in]     dumpHdl      Dump to write to
///
/// @returns OK, ISMRC_NotFound if the queue is not found or another ISMRC_
///          error.
//****************************************************************************
int32_t ieqn_dumpQueueByName(ieutThreadData_t *pThreadData,
                             const char *queueName,
                             iedmDumpHandle_t dumpHdl)
{
    int32_t rc = OK;
    ieqnQueue_t *queue = NULL;
    uint32_t queueNameHash = ieqn_generateQueueNameHash(queueName);
    ieqnQueueNamespace_t *queues = ismEngine_serverGlobal.queues;

    ieutTRACEL(pThreadData, queueName,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "queueName='%s'\n", __func__, queueName);

    ismEngine_getRWLockForRead(&queues->namesLock);

    rc = ieut_getHashEntry(queues->names,
                           queueName,
                           queueNameHash,
                           (void **)&queue);

    if (rc == OK)
    {
        assert(queue != NULL);

        __sync_add_and_fetch(&queue->useCount, 1);

        ismEngine_unlockRWLock(&queues->namesLock);

        ieqn_dumpQueue(pThreadData, queue, dumpHdl);

        ieqn_releaseQueue(pThreadData, queue);
    }
    else
    {
        ismEngine_unlockRWLock(&queues->namesLock);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief  Internal function to destroy a named queue
///
/// Destroy a queue with a given name
///
/// @param[in]     pQueueName      Queue name
/// @param[in]     discardMsgsOpt  Whether to discard messaged on the queue (purge).
/// @param[in]     adminRequest    Whether this request came from an admin caller
///
/// @remark If there are still active producers / consumers on the queue, the call
///         will fail with ISMRC_DestinationInUse.
///         If there are messages on the queue, and the ieqnDiscardMessages option
///         was not specified, the call will fail with ISMRC_DestinationNotEmpty.
///
/// @remark If the queue is temporary we disallow administrative requests to delete
///         the queue, returning ISMRC_DestTypeNotValid.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ieqn_destroyQueue(ieutThreadData_t *pThreadData,
                          const char *pQueueName,
                          ieqnDiscardMsgsOpt_t discardMsgsOpt,
                          bool adminRequest)
{
    int32_t rc = OK;
    ieqnQueue_t *queue = NULL;
    bool namesLocked = false;

    ieutTRACEL(pThreadData, pQueueName,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "pQueueName='%s'\n", __func__, pQueueName);

    assert(pQueueName != NULL);

    uint32_t queueNameHash = ieqn_generateQueueNameHash(pQueueName);

    ieqnQueueNamespace_t *queues = ismEngine_serverGlobal.queues;

    ismEngine_getRWLockForWrite(&queues->namesLock);
    namesLocked = true;

    rc = ieut_getHashEntry(queues->names,
                           pQueueName,
                           queueNameHash,
                           (void **)&queue);

    if (rc != OK) goto mod_exit;

    // Non-temporary queues are only removed from the namespace if they are not in use, and
    // are either empty, or we were instructed to discard messages.
    if (queue->temporary == false)
    {
        // If the queue is in use, return an error
        if (queue->useCount != 1)
        {
            rc = ISMRC_DestinationInUse;
            goto mod_exit;
        }

        // If not instructed to discard messages, check whether the queue contains messages
        if (discardMsgsOpt != ieqnDiscardMessages)
        {
            ismEngine_QueueStatistics_t stats;

            ieq_getStats(pThreadData, queue->queueHandle, &stats);

            // The stats indicate that there are messages still on the queue, so return an error
            if (stats.BufferedMsgs != 0)
            {
                rc = ISMRC_DestinationNotEmpty;
                goto mod_exit;
            }
        }
    }
    // Temporary queue cannot be deleted by an admin request
    else if (adminRequest == true)
    {
        rc = ISMRC_DestTypeNotValid;
        goto mod_exit;
    }

    // Remove the entry from the namespace
    ieut_removeHashEntry(pThreadData,
                         queues->names,
                         pQueueName,
                         queueNameHash);

    // No longer need the namespace locked
    ismEngine_unlockRWLock(&queues->namesLock);
    namesLocked = false;

    // Release the queue from the queue hash table
    ieqn_releaseQueue(pThreadData, queue);

mod_exit:

    if (namesLocked)
    {
        ismEngine_unlockRWLock(&queues->namesLock);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief  Add the specified queue to the specified group
///
/// @param[in]     pQueue          Queue to be added
/// @param[in,out] ppQueueGroup    The pointer to the group of queues
//****************************************************************************
void ieqn_addQueueToGroup(ieutThreadData_t *pThreadData,
                          ieqnQueue_t *pQueue,
                          ieqnQueue_t **ppQueueGroup)
{
    assert(pQueue->nextInGroup == NULL);

    ieutTRACEL(pThreadData, pQueue, ENGINE_FNC_TRACE, FUNCTION_ENTRY "pQueue=%p ppQueueGroup=%p\n", __func__,
               pQueue, ppQueueGroup);

    pQueue->nextInGroup = *ppQueueGroup;
    *ppQueueGroup = pQueue;

    ieutTRACEL(pThreadData, ppQueueGroup, ENGINE_FNC_TRACE, FUNCTION_EXIT, __func__);
}

//****************************************************************************
/// @brief  Remove the specified queue name from the specified group
///
/// @param[in]     pQueueName      Queue name to be remove
/// @param[in,out] ppQueueGroup    The pointer to the group of queues
///
/// @return OK on successful removal or ISMRC_NotFound
///
/// @remark Note that the act of removing a queue from a group does not remove
///         the queue from the namespace. Note also, we perform a linear search
///         of the queue group for the name specified.
//****************************************************************************
int32_t ieqn_removeQueueFromGroup(ieutThreadData_t *pThreadData,
                                  const char *pQueueName,
                                  ieqnQueue_t **ppQueueGroup)
{
    int32_t rc = ISMRC_NotFound;

    assert(ppQueueGroup != NULL);

    ieqnQueue_t *queue = *ppQueueGroup;
    ieqnQueue_t *prevQueue = NULL;

    ieutTRACEL(pThreadData, ppQueueGroup, ENGINE_FNC_TRACE, FUNCTION_ENTRY "pQueueName='%s' ppQueueGroup=%p\n", __func__,
               pQueueName, ppQueueGroup);

    while(NULL != queue)
    {
        if (strcmp(queue->queueName, pQueueName) == 0)
        {
            if (prevQueue == NULL)
            {
                *ppQueueGroup = queue->nextInGroup;
            }
            else
            {
                prevQueue->nextInGroup = queue->nextInGroup;
            }

            queue->nextInGroup = NULL;
            rc = OK;
            break;
        }

        prevQueue = queue;
        queue = queue->nextInGroup;
    }

    ieutTRACEL(pThreadData, queue,  ENGINE_FNC_TRACE, FUNCTION_EXIT "queue=%p, rc=%d\n", __func__, queue, rc);

    return rc;
}

//****************************************************************************
/// @brief  Request the destruction of all queues in a group of queues
///
/// @param[in]     pQueueGroup     The group of queues to delete
/// @param[in]     discardMsgsOpt  Whether to discard messaged on the queues
///                                (purge).
///
/// @remark At the moment, this function only operates on groups of temporary
///         queues.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ieqn_destroyQueueGroup(ieutThreadData_t *pThreadData,
                               ieqnQueue_t *pQueueGroup,
                               ieqnDiscardMsgsOpt_t discardMsgsOpt)
{
   ieqnQueue_t *queue = pQueueGroup;

   int32_t rc = OK;

   ieutTRACEL(pThreadData, pQueueGroup,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "pQueueGroup='%p'\n", __func__, pQueueGroup);

   while(NULL != queue)
   {
       ieqnQueue_t *nextQueue = queue->nextInGroup;

       assert(queue->temporary == true);

       int32_t destroy_rc = ieqn_destroyQueue(pThreadData, queue->queueName, discardMsgsOpt, false);

       // If we cannot destroy this particular queue FFDC, but carry on.
       if (destroy_rc != OK)
       {
           ieutTRACE_FFDC(1, false, "Unable to destroy queue from group", destroy_rc,
                          "queue",          queue,           sizeof(*queue)+strlen(queue->queueName),
                          "pQueueGroup",    pQueueGroup,     sizeof(*pQueueGroup),
                          "discardMsgsOpt", &discardMsgsOpt, sizeof(discardMsgsOpt),
                          NULL );

           // Cut this queue adrift from the group.
           queue->nextInGroup = NULL;

           // Report the failure up the line
           if (rc == OK) rc = destroy_rc;
       }

       queue = nextQueue;
   }

   ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

   return rc;
}

//****************************************************************************
/// @brief Function called by the engine callback for queues
///
/// @param[in]  objectIdentifier  Configuration object identifier
/// @param[in]  changedProps      A properties object (contains relevant properties)
/// @param[in]  changeType        Change type - refer to ism_ConfigChangeType_t
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int ieqn_queueConfigCallback(ieutThreadData_t *pThreadData,
                             char *objectIdentifier,
                             ism_prop_t *changedProps,
                             ism_ConfigChangeType_t changeType)
{
    int rc = OK;

    ieutTRACEL(pThreadData, changeType, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Get the queue name from the properties passed in
    const char *queueName = NULL;

    // Simplest way to find properties is to loop through looking for the prefix
    if (changedProps != NULL)
    {
        const char *propertyName;
        for (int32_t i = 0; ism_common_getPropertyIndex(changedProps, i, &propertyName) == 0; i++)
        {
            if (strncmp(propertyName,
                        ismENGINE_ADMIN_PREFIX_QUEUE_NAME,
                        strlen(ismENGINE_ADMIN_PREFIX_QUEUE_NAME)) == 0)
            {
                queueName = ism_common_getStringProperty(changedProps, propertyName);
                ieutTRACEL(pThreadData, queueName, ENGINE_NORMAL_TRACE, "QueueName='%s'\n", queueName);
                break;
            }
        }
    }

    // Didn't get a queue name
    if (queueName == NULL)
    {
        rc = ISMRC_InvalidParameter;
        goto mod_exit;
    }

    // The action taken varies depending on the requested changeType.
    switch(changeType)
    {
        case ISM_CONFIG_CHANGE_DELETE:
            rc = ieqn_destroyQueue(pThreadData,
                                   queueName,
                                   ism_common_getBooleanProperty(changedProps,
                                                                 ismENGINE_ADMIN_PROPERTY_DISCARDMESSAGES,
                                                                 ieqnKeepMessages),
                                   true);
            break;

        case ISM_CONFIG_CHANGE_PROPS:
            rc = ieqn_createQueue(pThreadData,
                                  queueName,
                                  multiConsumer,
                                  ismQueueScope_Server, NULL,
                                  changedProps,
                                  objectIdentifier,
                                  NULL);
            break;

        default:
            rc = ISMRC_InvalidOperation;
            break;
    }

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief  Release the named queue
///
/// Decrement the use count for the named queue.
///
/// @param[in]     queue  Queue to be released
//****************************************************************************
void ieqn_releaseQueue(ieutThreadData_t *pThreadData,
                       ieqnQueue_t *queue)
{
    ieutTRACEL(pThreadData, queue,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "queue=%p\n", __func__, queue);

    uint32_t oldCount = __sync_fetch_and_sub(&queue->useCount, 1);

    assert(oldCount != 0); // useCount just went negative

    // If this is the last release we should delete the queue.
    if (oldCount == 1)
    {
        (void)ieq_delete(pThreadData, &queue->queueHandle, false);
        iemem_freeStruct(pThreadData, iemem_queueNamespace, queue, queue->strucId);
    }

    ieutTRACEL(pThreadData, oldCount-1,  ENGINE_FNC_TRACE, FUNCTION_EXIT "useCount=%u\n", __func__, oldCount-1);

    return;
}

//****************************************************************************
/// @brief Register a consumer on a queue
///
/// @param[in]     queue     Queue upon which consumer is to be registered
/// @param[in]     consumer  Consumer to be registered
///
/// @see ieqn_unregisterConsumer
//****************************************************************************
static void ieqn_registerConsumer(ieutThreadData_t *pThreadData,
                                  ieqnQueue_t *queue,
                                  ismEngine_Consumer_t *consumer)
{
    assert(queue != NULL);

    ieutTRACEL(pThreadData, consumer, ENGINE_FNC_TRACE, FUNCTION_ENTRY "queueName='%s', consumer=%p\n", __func__,
               queue->queueName, consumer);

    assert(consumer->queueHandle == NULL);

    uint32_t newUseCount = __sync_add_and_fetch(&queue->useCount, 1);
    uint32_t newConsumerCount = __sync_add_and_fetch(&queue->consumerCount, 1);

    consumer->engineObject = queue;
    consumer->queueHandle = queue->queueHandle;

    ieutTRACEL(pThreadData, newConsumerCount, ENGINE_FNC_TRACE, FUNCTION_EXIT "useCount=%u, consumerCount=%u\n", __func__,
               newUseCount, newConsumerCount);

    return;
}

//****************************************************************************
/// @brief Unregister a consumer previously registered with ieqn_registerConsumer.
///
/// @param[in]     consumer  Consumer being unregistered
///
/// @see ieqn_registerConsumer
//****************************************************************************
void ieqn_unregisterConsumer(ieutThreadData_t *pThreadData,
                             ismEngine_Consumer_t *consumer)
{
    ieqnQueue_t *queue = (ieqnQueue_t *)(consumer->engineObject);

    ieutTRACEL(pThreadData, consumer, ENGINE_FNC_TRACE, FUNCTION_ENTRY "consumer=%p, queueName='%s'\n", __func__,
               consumer, queue->queueName);

    uint32_t oldConsumerCount = __sync_fetch_and_sub(&queue->consumerCount, 1);

    assert(oldConsumerCount != 0); // consumerCount just went negative

    // The queue may be deleted by this call, so no referencing it afterwards...
    ieqn_releaseQueue(pThreadData, queue);

    consumer->engineObject = NULL;

    ieutTRACEL(pThreadData, oldConsumerCount-1,  ENGINE_FNC_TRACE, FUNCTION_EXIT "consumerCount=%u\n", __func__, oldConsumerCount-1);

    return;
}

//****************************************************************************
/// @brief Register a producer on a queue
///
/// @param[in]     queue     Queue upon which producer is to be registered
/// @param[in]     producer  Producer to be registered
///
/// @see ieqn_unregisterProducer
//****************************************************************************
static void ieqn_registerProducer(ieutThreadData_t *pThreadData,
                                  ieqnQueue_t *queue,
                                  ismEngine_Producer_t *producer)
{
    assert(queue != NULL);

    ieutTRACEL(pThreadData, producer, ENGINE_FNC_TRACE, FUNCTION_ENTRY "queueName='%s', producer=%p\n", __func__,
               queue->queueName, producer);

    assert(producer->queueHandle == NULL);

    uint32_t newUseCount = __sync_add_and_fetch(&queue->useCount, 1);
    uint32_t newProducerCount = __sync_add_and_fetch(&queue->producerCount, 1);

    producer->engineObject = queue;
    producer->queueHandle = queue->queueHandle;

    ieutTRACEL(pThreadData, newProducerCount, ENGINE_FNC_TRACE, FUNCTION_EXIT "useCount=%u, producerCount=%u\n", __func__,
               newUseCount, newProducerCount);

    return;
}

//****************************************************************************
/// @brief Unregister a producer previously registered with ieqn_registerProducer.
///
/// @param[in]     producer  Producer being unregistered
///
/// @see ieqn_registerProducer
//****************************************************************************
void ieqn_unregisterProducer(ieutThreadData_t *pThreadData,
                             ismEngine_Producer_t *producer)
{
    ieqnQueue_t *queue = (ieqnQueue_t *)(producer->engineObject);

    ieutTRACEL(pThreadData, producer, ENGINE_FNC_TRACE, FUNCTION_ENTRY "producer=%p, queueName='%s'\n", __func__,
               producer, queue->queueName);

    uint32_t oldProducerCount = __sync_fetch_and_sub(&queue->producerCount, 1);

    assert(oldProducerCount != 0); // producerCount just went negative

    // The queue may be deleted by this call, so no referencing it afterwards...
    ieqn_releaseQueue(pThreadData, queue);

    producer->engineObject = NULL;
    producer->queueHandle = NULL;

    ieutTRACEL(pThreadData, oldProducerCount-1,  ENGINE_FNC_TRACE, FUNCTION_EXIT "producerCount=%u\n", __func__, oldProducerCount-1);

    return;
}

//****************************************************************************
/// @brief  Retrieve the statsitics for a named queue
///
/// @param[in]     pQueueName         Queue name
/// @param[out]    pStats             Statistics
///
/// @remark This does not retrieve (or update) the activity delta stat.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ieqn_getQueueStats(ieutThreadData_t *pThreadData,
                           const char  *pQueueName,
                           ismEngine_QueueStatistics_t *pQStats)
{
    int32_t rc = OK;
    ieqnQueue_t *queue = NULL;

    ieutTRACEL(pThreadData, pQStats, ENGINE_FNC_TRACE, FUNCTION_ENTRY "pQueueName='%s'\n", __func__,
               pQueueName);

    assert(pQueueName != NULL);

    uint32_t queueNameHash = ieqn_generateQueueNameHash(pQueueName);

    ieqnQueueNamespace_t *queues = ismEngine_serverGlobal.queues;

    ismEngine_getRWLockForRead(&queues->namesLock);

    rc = ieut_getHashEntry(queues->names,
                           pQueueName,
                           queueNameHash,
                           (void **)&queue);

    if (rc == OK)
    {
        ieq_getStats(pThreadData, queue->queueHandle, pQStats);
    }

    ismEngine_unlockRWLock(&queues->namesLock);

    ieutTRACEL(pThreadData, queue,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief  Retrieve the handle for a named queue
///
/// @param[in]     pQueueName  Queue name
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
ismQHandle_t ieqn_getQueueHandle(ieutThreadData_t *pThreadData, const char  *pQueueName)
{
    int32_t rc = OK;
    ieqnQueue_t *queue = NULL;
    ismQHandle_t queueHandle = NULL;

    ieutTRACEL(pThreadData, pQueueName, ENGINE_FNC_TRACE, FUNCTION_ENTRY "pQueueName='%s'\n", __func__,
               pQueueName);

    assert(pQueueName != NULL);

    uint32_t queueNameHash = ieqn_generateQueueNameHash(pQueueName);

    ieqnQueueNamespace_t *queues = ismEngine_serverGlobal.queues;

    ismEngine_getRWLockForRead(&queues->namesLock);

    rc = ieut_getHashEntry(queues->names,
                           pQueueName,
                           queueNameHash,
                           (void **)&queue);

    if (rc == OK)
    {
        queueHandle = queue->queueHandle;
    }

    ismEngine_unlockRWLock(&queues->namesLock);

    ieutTRACEL(pThreadData, queueHandle,  ENGINE_FNC_TRACE, FUNCTION_EXIT "Handle=%p\n", __func__, queueHandle);

    return queueHandle;
}

//****************************************************************************
/// @brief  Determine whether the specified queue is a temporary queue
///
/// @param[in]     pQueueName  Queue name
/// @param[out]    pTemporary  If the queue exists, whether it is temporary
/// @param[out]    ppCreator   (Optional) Pointer in which to return the client
///                            state of the creator (only valid for a temporary
///                            queue).
///
/// @return OK if the queue exists otherwise an ISMRC_ value.
//****************************************************************************
int32_t ieqn_isTemporaryQueue(const char *pQueueName,
                              bool *pTemporary,
                              ismEngine_ClientState_t **ppCreator)
{
    int32_t rc;
    ieqnQueue_t *queue = NULL;

    assert(pQueueName != NULL);
    assert(pTemporary != NULL);

    uint32_t queueNameHash = ieqn_generateQueueNameHash(pQueueName);

    ieqnQueueNamespace_t *queues = ismEngine_serverGlobal.queues;

    ismEngine_getRWLockForRead(&queues->namesLock);

    rc = ieut_getHashEntry(queues->names,
                           pQueueName,
                           queueNameHash,
                           (void **)&queue);

    if (rc == OK)
    {
        *pTemporary = queue->temporary;

        // If requested, return the creator's client state pointer
        if (queue->temporary && NULL != ppCreator)
        {
            *ppCreator = queue->creator;
        }
    }
    else
    {
        if (ismEngine_serverGlobal.disableAutoQueueCreation == false)
        {
            rc = OK;
            *pTemporary = false;
        }
        else
        {
            rc = ISMRC_DestNotValid;
            ism_common_setErrorData(rc, "%s", pQueueName);
        }
    }

    ismEngine_unlockRWLock(&queues->namesLock);

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief Write the descriptions of queue namespace structures to the dump
///
/// @param[in]     dumpHdl  Pointer to a dump structure
//****************************************************************************
void ieqn_dumpWriteDescriptions(iedmDumpHandle_t dumpHdl)
{
    iedmDump_t *dump = (iedmDump_t *)dumpHdl;

    iedm_describe_ieqnQueue_t(dump->fp);
}

//****************************************************************************
/// @brief Generate a hash value for a specified key string used in the queue
///        namespace.
///
/// @param[in]     key  The key for which to generate a hash value
///
/// @remark This is a version of the xor djb2 hashing algorithm
///
/// @return The hash value
//****************************************************************************
static uint32_t ieqn_generateQueueNameHash(const char *key)
{
    uint32_t keyHash = 5381;
    char curChar;

    while((curChar = *key++))
    {
        keyHash = (keyHash * 33) ^ curChar;
    }

    return keyHash;
}
