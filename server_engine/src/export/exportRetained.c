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
/// @file  exportRetained.c
/// @brief Export / Import functions for retained messages
//*********************************************************************
#define TRACE_COMP Engine

#include <assert.h>

#include "exportRetained.h"
#include "topicTree.h"
#include "topicTreeInternal.h"
#include "msgCommon.h"
#include "exportMessage.h"
#include "destination.h"


typedef struct tag_ieieAsyncRetainedContext_t {
    ieieImportResourceControl_t *control;
    uint64_t                     dataId;
    ismEngine_Message_t         *message;
} ieieAsyncRetainedContext_t;

//****************************************************************************
/// @internal
///
/// @brief  Export the information for retained messages whose topic matches
///         the specified topic regular expression.
///
/// @param[in]     control        ieieExportResourceControl_t for this export
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
int32_t ieie_exportRetainedMsgs(ieutThreadData_t *pThreadData,
                                ieieExportResourceControl_t *control)
{
    int32_t rc = OK;

    assert(control != NULL);
    assert(control->file != NULL);
    assert(control->topic != NULL);

    ieutTRACEL(pThreadData, control->topic, ENGINE_FNC_TRACE, FUNCTION_ENTRY "topic='%s' outFile=%p\n", __func__,
               control->topic, control->file);

    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    uint32_t maxNodes = 0;
    uint32_t nodeCount = 0;
    iettTopicNode_t **topicNodes = NULL;
    ismEngine_MessageHandle_t *foundMessages = NULL;
    uint32_t foundMessageCount = 0;
    uint32_t foundMessageMax = 0;

    uint32_t totalRetainedFound = 0;

    // Static analysed topic string to pass to iett_findMatchingTopicsNodes
    iettTopic_t topic = {0};
    topic.topicString = control->topic;
    topic.destinationType = ismDESTINATION_REGEX_TOPIC;
    topic.regex = control->regexTopic;

    // Lock the topic tree for read access
    ismEngine_getRWLockForRead(&tree->topicsLock);

    rc = iett_findMatchingTopicsNodes(pThreadData,
                                      tree->topics, false,
                                      &topic,
                                      0, 0, 0,
                                      NULL, &maxNodes, &nodeCount, &topicNodes);

    if (rc == OK)
    {
        const uint32_t nowExpiry = ism_common_nowExpire();

        // Sort the nodes by timestamp so that they are in the natural order for quick import
        iett_sortTopicNodesByTimestamp(pThreadData, topicNodes, nodeCount);

        // Go through all of the nodes returned building a list of retained messages
        for(uint32_t i=0; i<nodeCount; i++)
        {
            ismEngine_Message_t *pMessage = topicNodes[i]->currRetMessage;

            // If there is a message consider adding it to our list
            if (pMessage != NULL)
            {
                // Only add the message to the list if it has not expired.
                if ((pMessage->Header.Expiry == 0) || (pMessage->Header.Expiry > nowExpiry))
                {
                    if (foundMessageCount == foundMessageMax)
                    {
                        ismEngine_MessageHandle_t *newFoundMessages;
                        newFoundMessages = iemem_realloc(pThreadData,
                                                         IEMEM_PROBE(iemem_topicsTree, 12),
                                                         foundMessages,
                                                         sizeof(ismEngine_MessageHandle_t)*(foundMessageMax+10));

                        if (newFoundMessages == NULL)
                        {
                            rc = ISMRC_AllocateError;
                            ism_common_setError(rc);

                            // Don't process the list we built up already
                            foundMessageCount = 0;
                            break;
                        }

                        foundMessages = newFoundMessages;
                        foundMessageMax += 10;
                    }

                    foundMessages[foundMessageCount++] = pMessage;
                }
            }
        }

        // Increment the useCount of found messages while we're holding the lock
        for(uint32_t i=0; i<foundMessageCount; i++)
        {
            iem_recordMessageUsage(foundMessages[i]);
        }
    }
    else
    {
        // Don't expect to see ISMRC_NotFound for REGEX. If that changes,
        // just set it to OK if rc == ISMRC_NotFound.
        assert(rc != ISMRC_NotFound);
    }

    // Release the lock
    ismEngine_unlockRWLock(&tree->topicsLock);

    if (foundMessageCount != 0)
    {
        // Initialize to zero to ensure unused fields are obvious (and usable in the future)
        ieieRetainedMsgInfo_t retainedInfo = {0};

        retainedInfo.Version = ieieRETAINEDMSG_CURRENT_VERSION;

        // Export the messages & retained information.
        for(uint32_t i=0; i<foundMessageCount; i++)
        {
            ismEngine_Message_t *message = foundMessages[i];

            if (rc == OK)
            {
                rc = ieie_exportMessage(pThreadData, message, control, true);

                if (rc == OK)
                {
                    rc = ieie_writeExportRecord(pThreadData,
                                                control,
                                                ieieDATATYPE_EXPORTEDRETAINEDMSG,
                                                (uint64_t)message,
                                                (uint32_t)sizeof(retainedInfo),
                                                (void *)&retainedInfo);

                    if (rc == OK) totalRetainedFound++;
                }
            }
            else
            {
                iem_releaseMessage(pThreadData, message);
            }
        }
    }

    // Free the arrays
    if (foundMessages != NULL) iemem_free(pThreadData, iemem_topicsTree, foundMessages);
    if (topicNodes != NULL) iemem_free(pThreadData, iemem_topicsQuery, topicNodes);

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d totalRetainedFound=%u\n", __func__, rc, totalRetainedFound);

    return rc;
}

static const char *ieie_getTopicStringFromMessage( ieutThreadData_t *pThreadData
                                                 , ismEngine_Message_t *message)
{
    // Get the topic string out of the message
    size_t proplen = 0;
    char *propp = NULL;
    for (uint32_t i = 0; i < message->AreaCount; i++)
    {
        if (message->AreaTypes[i] == ismMESSAGE_AREA_PROPERTIES)
        {
            proplen = message->AreaLengths[i];
            propp = (char *) message->pAreaData[i];
            break;
        }
    }

    // We have properties
    assert((proplen != 0) && (propp != NULL));

    concat_alloc_t  props = {propp, proplen, proplen};
    ism_field_t field;

    field.val.s = NULL;
    ism_common_findPropertyID(&props, ID_Topic, &field);
    const char * topicString = field.val.s;

    return topicString;
}

void ieie_completeAsync_importRetainedMsg(
    int32_t                         retcode,
    void *                          handle,
    void *                          pContext)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine( NULL);

    ieieAsyncRetainedContext_t *context = (ieieAsyncRetainedContext_t *)pContext;
    ieieImportResourceControl_t *control = context->control;

    if (retcode != OK)
    {
        const char *topicString = ieie_getTopicStringFromMessage(pThreadData, context->message);

        assert(topicString != NULL);

        char humanIdentifier[strlen(topicString)+strlen("Topic:")+1];
        sprintf(humanIdentifier, "Topic:%s", topicString);

        ieie_recordImportError( pThreadData
                              , control
                              , ieieDATATYPE_EXPORTEDRETAINEDMSG
                              , context->dataId
                              , humanIdentifier
                              , retcode);
    }

    if (context->message != NULL)
    {
        //We didn't release the message before we went async so we could use the topicstring out of it
        //in the above error report if necessary
        iem_releaseMessage(pThreadData, context->message);
    }

    ieie_finishImportRecord(pThreadData, control, ieieDATATYPE_EXPORTEDRETAINEDMSG);
    (void)ieie_importTaskFinish( pThreadData
                               , control
                               , true
                               , NULL);

    ieut_leavingEngine(pThreadData);
}

//****************************************************************************
/// @internal
///
/// @brief  Import (publish) a retained message.
///
/// @param[in]     control        ieieImportResourceControl_t for this import
/// @param[in]     dataId         dataId (for retained msgs, the dataId of the
///                               message itself)
/// @param[in]     data           ieieRetainedMsgInfo_t
/// @param[in]     dataLen        sizeof(ieieRetainedMsgInfo_t)
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
int32_t ieie_importRetainedMsg(ieutThreadData_t *pThreadData,
                               ieieImportResourceControl_t *control,
                               uint64_t dataId,
                               uint8_t *data,
                               size_t dataLen)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, dataId, ENGINE_FNC_TRACE, FUNCTION_ENTRY "dataId=0x%0lx\n", __func__, dataId);

    assert(((ieieRetainedMsgInfo_t *)data)->Version == ieieRETAINEDMSG_CURRENT_VERSION);

    ismEngine_Message_t *message = NULL;
    const char *topicString = NULL;

    rc = ieie_findImportedMessage(pThreadData,
                                  control,
                                  dataId,
                                  &message);

    if (rc != OK)
    {
        ism_common_setError(rc);
        goto mod_exit;
    }

    topicString = ieie_getTopicStringFromMessage(pThreadData, message);
    assert(topicString != NULL);

    assert(iett_validateTopicString(pThreadData, topicString, iettVALIDATE_FOR_PUBLISH) == true);

    // Now actually publish the message
    ietrAsyncTransactionDataHandle_t hAsyncData = NULL;

    ieieAsyncRetainedContext_t context = {control, dataId, message};

    rc = ieds_publish(pThreadData,
                      NULL,
                      topicString,
                      iedsPUBLISH_OPTION_POTENTIAL_REPUBLISH,
                      NULL,
                      message,
                      0,
                      NULL,
                      sizeof(context),
                      &hAsyncData);

    if (rc == ISMRC_NeedStoreCommit)
    {
        //The publish wants to go async.... get ready
        rc = setupAsyncPublish( pThreadData
                              , NULL
                              , NULL
                              , &context
                              , sizeof(context)
                              , ieie_completeAsync_importRetainedMsg
                              , &hAsyncData);

        if (rc == ISMRC_AsyncCompletion)
        {
            goto mod_exit;
        }
    }


mod_exit:

    if (rc != OK && rc != ISMRC_AsyncCompletion)
    {
        char humanIdentifierBuffer[(topicString == NULL ? 0 : strlen(topicString))+strlen("Topic:")+1];
        char *humanIdentifier = NULL; // Initialise the point to be NULL

        if (topicString != NULL)
        {
            sprintf(humanIdentifierBuffer, "Topic:%s", topicString);
            humanIdentifier = humanIdentifierBuffer;
        }
        else
        {
            humanIdentifier = NULL;
        }

        ieie_recordImportError( pThreadData
                              , control
                              , ieieDATATYPE_EXPORTEDRETAINEDMSG
                              , dataId
                              , humanIdentifier
                              , rc);
    }

    //We don't release the message if we go async so we can read the topicstring out of it if necessary
    if (rc != ISMRC_AsyncCompletion && message != NULL) iem_releaseMessage(pThreadData, message);

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* End of exportRetained.c                                           */
/*                                                                   */
/*********************************************************************/
