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

#include <mqcInternal.h>
#include <mqcbridgeExternal.h>
#include <ismmonitoringobjs.h>
#include <engine.h>
#include <monserialization.h>

#include <fcntl.h>

/*
 * Monitoring action types
 */
typedef enum {
	ismMON_STAT_None          = 0,
	ismMON_STAT_DestinationMappingRule   = 1,
} ismMonitoringStatType_t;

XAPI int mqc_monitoring_getRuleStats(char * action,
                                     const char *locale,
                                     ism_json_parse_t * inputJSONObj,
                                     concat_alloc_t * outputBuffer);

DEFINE_DESTINATIONMAPPINGRULE_MONITORING_OBJ(XmqcStatistics_t);

/* Convert the text form of the StatType (pStatTypeString) into an enum */

ruleStatistic_t ruleStatTypeToEnum(char *pStatTypeString)
{
    ruleStatistic_t requestedRuleStatistic = mqcRULE_STAT_None;

    if (!strcasecmp(pStatTypeString, "CommitCount"))
        requestedRuleStatistic = mqcRULE_STAT_CommitCount;
    else if (!strcasecmp(pStatTypeString, "RollbackCount"))
        requestedRuleStatistic = mqcRULE_STAT_RollbackCount;
    else if (!strcasecmp(pStatTypeString, "PersistentCount"))
        requestedRuleStatistic = mqcRULE_STAT_PersistentCount;
    else if (!strcasecmp(pStatTypeString, "NonpersistentCount"))
        requestedRuleStatistic = mqcRULE_STAT_NonpersistentCount;
    else if (!strcasecmp(pStatTypeString, "CommittedMessageCount"))
        requestedRuleStatistic = mqcRULE_STAT_CommittedMessageCount;
    else if (!strcasecmp(pStatTypeString, "Status"))
            requestedRuleStatistic = mqcRULE_STAT_Status;

    return requestedRuleStatistic;
}

/* Convert the text form of the RuleType (pRuleType) to an enum. */
mqcRuleType_t ruleTypeToEnum(char *pRuleType)
{
    mqcRuleType_t requestedRuleType = mqcRULE_None;

    if (*pRuleType == 'I' || *pRuleType == 'i')
    {
        if (!strcasecmp(pRuleType, "IMATopicToMQQueue"))
            requestedRuleType = mqcRULE_FROM_IMATOPIC_TO_MQQUEUE;
        else if (!strcasecmp(pRuleType, "IMATopicToMQTopic"))
            requestedRuleType = mqcRULE_FROM_IMATOPIC_TO_MQTOPIC;
        else if (!strcasecmp(pRuleType, "IMATopicSubtreeToMQQueue"))
            requestedRuleType = mqcRULE_FROM_IMAWILDTOPIC_TO_MQQUEUE;
        else if (!strcasecmp(pRuleType, "IMATopicSubtreeToMQTopic"))
            requestedRuleType = mqcRULE_FROM_IMAWILDTOPIC_TO_MQTOPIC;
        else if (!strcasecmp(pRuleType, "IMATopicSubtreeToMQTopicSubtree"))
            requestedRuleType = mqcRULE_FROM_IMAWILDTOPIC_TO_MQWILDTOPIC;
        else if (!strcasecmp(pRuleType, "IMAQueueToMQQueue"))
            requestedRuleType = mqcRULE_FROM_IMAQUEUE_TO_MQQUEUE;
        else if (!strcasecmp(pRuleType, "IMAQueueToMQTopic"))
            requestedRuleType = mqcRULE_FROM_IMAQUEUE_TO_MQTOPIC;
    }
    else
    {
        if (!strcasecmp(pRuleType, "MQQueueToIMATopic"))
            requestedRuleType = mqcRULE_FROM_MQQUEUE_TO_IMATOPIC;
        else if (!strcasecmp(pRuleType, "MQTopicToIMATopic"))
            requestedRuleType = mqcRULE_FROM_MQTOPIC_TO_IMATOPIC;
        else if (!strcasecmp(pRuleType, "MQTopicSubtreeToIMATopic"))
            requestedRuleType = mqcRULE_FROM_MQWILDTOPIC_TO_IMATOPIC;
        else if (!strcasecmp(pRuleType, "MQTopicSubtreeToIMATopicSubtree"))
            requestedRuleType = mqcRULE_FROM_MQWILDTOPIC_TO_IMAWILDTOPIC;
        else if (!strcasecmp(pRuleType, "MQQueueToIMAQueue"))
            requestedRuleType = mqcRULE_FROM_MQQUEUE_TO_IMAQUEUE;
        else if (!strcasecmp(pRuleType, "MQTopicToIMAQueue"))
            requestedRuleType = mqcRULE_FROM_MQTOPIC_TO_IMAQUEUE;
        else if (!strcasecmp(pRuleType, "MQTopicSubtreeToIMAQueue"))
            requestedRuleType = mqcRULE_FROM_MQWILDTOPIC_TO_IMAQUEUE;
        else if (!strcasecmp(pRuleType, "Any"))
            requestedRuleType = mqcRULE_Any;
    }

    return requestedRuleType;
}

int copyMqcStatistics(mqcStatistics_t *pNew, mqcRuleQM_t *pRuleQM, const char *locale)
{

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    TRACE(MQC_MQAPI_TRACE,
          "Rule name = %s\n",
          pRuleQM -> pRule -> pName);
    TRACE(MQC_MQAPI_TRACE,
          "QM connection name = %s\n",
          pRuleQM -> pQM -> pName);

    if ((pNew != NULL) && (pRuleQM != NULL))
    {
        pNew -> RuleName = pRuleQM -> pRule -> pName;
        pNew -> QueueManagerConnection = pRuleQM -> pQM -> pName;
        pNew -> nonpersistentCount = pRuleQM -> nonpersistentCount;
        pNew -> persistentCount = pRuleQM -> persistentCount;
        pNew -> commitCount = pRuleQM -> commitCount;
        pNew -> rollbackCount = pRuleQM -> rollbackCount;
        pNew -> committedMessageCount = pRuleQM -> committedMessageCount;

        if (pRuleQM -> flags & RULEQM_STARTING)
        {
            pNew -> status = mqcRule_Starting;
        }
        else if (pRuleQM -> flags & RULEQM_DISABLED)
        {
            pNew -> status = mqcRule_Disabled;
        }
        else if ((pRuleQM -> flags & RULEQM_RECONNECTING) &&
                 !(pRuleQM -> flags & RULEQM_CONNECTED))
        {
            pNew -> status = mqcRule_Reconnecting;
        }
        else
        {
            pNew -> status = mqcRule_Enabled;
        }

        TRACE(MQC_MQAPI_TRACE,
              "Rule/QM status = %d\n",
              pNew -> status);

        if (pNew -> status == mqcRule_Enabled)
        {
            pNew -> OtherRC = 0;
            pNew -> MQConnectivityRC = 0; /* MQ Connectivity return code */
            pNew -> pOtherRCSymbol = NULL;
            pNew -> pErrorQueueName = NULL;
            pNew -> pSubName = NULL;
            pNew -> pErrorAPI = NULL;
            pNew -> pExpandedMessage = NULL;
        }
        else
        {
            const char *repl[6];
            int32_t errorMsgId = 0, replacementCount = 0;

            pNew -> OtherRC = pRuleQM -> OtherRC;
            pNew -> MQConnectivityRC = pRuleQM -> MQConnectivityRC;
            pNew -> pOtherRCSymbol = pRuleQM -> pOtherRCSymbol;
            pNew -> pErrorQueueName = pRuleQM -> errorQueueName;
            pNew -> pSubName = pRuleQM -> subName;
            pNew -> pErrorAPI = pRuleQM -> errorAPI;

            if ((mqcMQConnectivity.flags & MQC_DISABLED) ||
                (mqcMQConnectivity.flags & MQC_RECONNECTING))
            {
                errorMsgId  = mqcMQConnectivity.ima.errorMsgId;

                repl[0] = mqcMQConnectivity.ima.errorInsert[0];
                repl[1] = mqcMQConnectivity.ima.errorInsert[1];
                repl[2] = mqcMQConnectivity.ima.errorInsert[2];

                replacementCount = 3;
            }
            else if ((pRuleQM -> pRule -> flags & RULE_DISABLED) ||
                     (pRuleQM -> pRule -> flags & RULE_RECONNECTING))
            {
                errorMsgId = pRuleQM -> pRule -> errorMsgId;

                repl[0] = pRuleQM -> pRule -> pName;
                repl[1] = pRuleQM -> pRule -> errorInsert[0];
                repl[2] = pRuleQM -> pRule -> errorInsert[1];

                replacementCount = 3;
            }
            else
            {
                errorMsgId = pRuleQM -> errorMsgId;

                // The inserts to use vary by message id
                switch(errorMsgId)
                {
                case RC_InvalidMessage:
                    repl[replacementCount++] = pRuleQM -> errorInsert[0];
                    break;
                default:
                    repl[replacementCount++] = pRuleQM -> pRule -> pName;
                    repl[replacementCount++] = pRuleQM -> pQM -> pName;
                    repl[replacementCount++] = pRuleQM -> errorInsert[0];
                    repl[replacementCount++] = pRuleQM -> errorInsert[1];
                    repl[replacementCount++] = pRuleQM -> errorInsert[2];
                    repl[replacementCount++] = pRuleQM -> errorInsert[3];
                    break;
                }
            }

            // If we have a message, let's get it and format it
            if (errorMsgId != 0)
            {
                char msgID[12];
                char lbuf[1024];
                int xlen;

                sprintf(msgID, "CWLNA%04d", errorMsgId);

                if (ism_common_getMessageByLocale(msgID,
                                                  locale,
                                                  lbuf,
                                                  sizeof(lbuf),
                                                  &xlen) != NULL)
                {
                    size_t msgLength = (size_t)xlen;

                    for(int32_t i=0; i<replacementCount; i++)
                    {
                        msgLength += strlen(repl[i]);
                    }

                    msgLength += 50; // Add some space

                    pNew -> pExpandedMessage = mqc_malloc(msgLength);

                    if (pNew -> pExpandedMessage != NULL)
                    {
                        pNew -> pExpandedMessage[0] = '\0';

                        ism_common_formatMessage(pNew -> pExpandedMessage,
                                                 msgLength,
                                                 lbuf,
                                                 repl,
                                                 replacementCount);
                    }
                }
                else
                {
                    pNew -> pExpandedMessage = NULL;
                }
            }
        }

        return 0;
    }
    else
    {
        return 1;
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
}

bool statisticGT(mqcStatistics_t *pLeft,
                 mqcStatistics_t *pRight,
                 ruleStatistic_t type)
{
    bool result = false;

    switch (type)
    {
    case mqcRULE_STAT_CommitCount:
        result = (pLeft -> commitCount > pRight -> commitCount);
        break;
    case mqcRULE_STAT_RollbackCount:
        result = (pLeft -> rollbackCount > pRight -> rollbackCount);
        break;
    case mqcRULE_STAT_PersistentCount:
        result = (pLeft -> persistentCount > pRight -> persistentCount);
        break;
    case mqcRULE_STAT_NonpersistentCount:
        result = (pLeft -> nonpersistentCount > pRight -> nonpersistentCount);
        break;
    case mqcRULE_STAT_CommittedMessageCount:
        result = (pLeft -> committedMessageCount > pRight -> committedMessageCount);
        break;
    case mqcRULE_STAT_Status:
    default:
        break;
    }
    return result;
}

void freeResultList(mqcStatistics_t *pCurrent)
{
    while (pCurrent != NULL)
    {
        mqcStatistics_t *pPrevious = pCurrent;
        pCurrent = pCurrent -> pNext;
        if (pPrevious -> pExpandedMessage != NULL)
            mqc_free(pPrevious -> pExpandedMessage);
        mqc_free(pPrevious);
    }
}


/*
 * Get monitoring action type
 */
static int ism_monitoring_getStatType(char *actionString) {
	if ( !strcasecmp(actionString, "DestinationMappingRule"))
		return ismMON_STAT_DestinationMappingRule;
	return ismMON_STAT_None;
}
/*
 * Interface for protocol - to send Monitoring request and get results back
 * Note: Status return must be via the fifth (int *rc) parameter and not
 * the function return, although it probably does no harm to do both.
 */

XAPI int ism_process_mqc_monitoring_action(const char * locale,
    const char *inpbuf,
    int inpbuflen,
    concat_alloc_t *output_buffer,
    int *rc)
{
    char *action = NULL;
    char *user = NULL;
    char  rbuf[1024];

    char  lbuf[1024];

    monitorStatistic_t statType = mqcMON_STAT_None;

    if (inpbuf == NULL)
    {
        *rc = ISMRC_NullPointer;
        ism_common_setError(*rc);
        return ISMRC_NullPointer;
    }

    if (*inpbuf != '{')
    {
        *rc = ISMRC_BadRESTfulRequest;
        ism_common_setErrorData(*rc, "%s", inpbuf);
        return ISMRC_BadRESTfulRequest;
    }

    char *input_buffer = mqc_malloc(inpbuflen + 1);
    if (input_buffer == NULL)
    {
        mqc_allocError("malloc", inpbuflen + 1, errno);
        *rc = ISMRC_AllocateError;
        ism_common_setError(*rc);
        return ISMRC_AllocateError;
    }

    memcpy(input_buffer, inpbuf, inpbuflen);
    input_buffer[inpbuflen] = 0;

    TRACE(MQC_HIGH_TRACE, "Monitoring request buffer: \n%s\n", input_buffer);

    ism_json_parse_t parseobj = { 0 };
    ism_json_entry_t ents[20];
    char *tmpbuf = input_buffer;

    parseobj.ent = ents;
    parseobj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));
    parseobj.source = (char *) input_buffer;
    parseobj.src_len = inpbuflen + 1;
    ism_json_parse(&parseobj);

    memset(rbuf, 0, sizeof(rbuf));
    memset(lbuf, 0, sizeof(lbuf));

    if (parseobj.rc)
    {
        /* RC_MonitorParseError = 7025 */
        LOG(ERROR,
            MQConnectivity,
            7025,
            "%-s%d",
            "Failed to parse monitoring request {0}: RC={1}.",
            tmpbuf,
            parseobj.rc);

        mqc_free(input_buffer);

        *rc = ISMRC_ParseError;
        return ISMRC_ParseError;
    }
    else
    {
        action = (char *) ism_json_getString(&parseobj, "Action");
        user = (char *) ism_json_getString(&parseobj, "User");

        const char *specifiedLocale = (char *) ism_json_getString(&parseobj, "Locale");

        // If the request specifies a locale, use it, otherwise use the default
        if (specifiedLocale != NULL)
        {
            locale = specifiedLocale;
        }
    }

    if (!action || !*action)
    {
        ism_common_getErrorStringByLocale(6502,
                                          locale,
                                          lbuf,
                                          sizeof(lbuf));
        /* RC_MonitorActionUnsupported = 7026 */
        LOG(ERROR,
            Monitoring,
            7026,
            "%-s",
            "The monitoring action {0} is not supported.",
            tmpbuf);

        mqc_free(input_buffer);
        *rc = 0;
        return 0;
    }

    /* Change this to audit log? */
    TRACE(MQC_NORMAL_TRACE,
          "Monitoring: action=%s user=%-s\n",
          action,
          user ? user : "");

    statType = ism_monitoring_getStatType(action);

    /* Process Monitoring received in JSON format */
    switch (statType)
    {
	    case mqcMON_STAT_DestinationMappingRule:
	    {
	        *rc = mqc_monitoring_getRuleStats(action, locale, &parseobj, output_buffer);
	        if (*rc != 0)
	        {
	            return *rc;
	        }

	        break;
	    }


	    default:
	    {
	        *rc = ISMRC_NotFound;

	        break;
	    }
    }

    mqc_free(input_buffer);

    if (output_buffer->used > 0)
    {
        *rc = 0;
        return -1;
    }
    else
    {
        return *rc;
    }
}

XAPI int mqc_monitoring_getRuleStats(char *action,
                                     const char *locale,
                                     ism_json_parse_t *inputJSONObj,
                                     concat_alloc_t *outputBuffer)
{
    int rc = ISMRC_OK;
    mqcStatistics_t *pResults = NULL;

    ruleStatistic_t requestedRuleStatistic = mqcRULE_STAT_None;
    mqcRuleType_t requestedRuleType = mqcRULE_None;

    mqcRule_t *pRule = mqcMQConnectivity.pRuleList;
    mqcRuleQM_t *pRuleQM = NULL;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    /* Get input data from JSON object */
    char *pRuleName = (char *) ism_json_getString(inputJSONObj, "Name");
    char *pRuleType = (char *) ism_json_getString(inputJSONObj, "RuleType");
    char *pAssociation = (char *) ism_json_getString(inputJSONObj,
                                                     "Association");
    char *pStatTypeString = (char *) ism_json_getString(inputJSONObj,
                                                        "StatType");
    uint32_t maxResults = (uint32_t) ism_json_getInt(inputJSONObj,
                                                     "ResultCount",
                                                     25);

    TRACE(MQC_MQAPI_TRACE, "Rule Name = '%s'\n", (pRuleName == NULL) ? "NULL"
            : pRuleName);
    TRACE(MQC_MQAPI_TRACE, "Rule Type = '%s'\n", (pRuleType == NULL) ? "NULL"
            : pRuleType);
    TRACE(MQC_MQAPI_TRACE,
          "Association = '%s'\n",
          (pAssociation == NULL) ? "NULL" : pAssociation);
    TRACE(MQC_MQAPI_TRACE,
          "StatType = '%s'\n",
          (pStatTypeString == NULL) ? "NULL" : pStatTypeString);
    TRACE(MQC_MQAPI_TRACE, "ResultCount = '%d'\n", maxResults);
    TRACE(MQC_MQAPI_TRACE, "Locale = '%s'\n", locale);

    uint32_t numberOfResults = 0;

    if ( !pRuleName && !pRuleType && !pAssociation && !pStatTypeString ) {
    	ism_common_allocBufferCopyLen(outputBuffer, "{ \"Version\":\"v1\", \"DestinationMappingRule\": [ ] }", 49);
    	return rc;
    }

    if (!pRuleName || (pRuleName && (*pRuleName == '\0')))
    {
        /* TODO: Log error message and return invalid rule name - may have to create an return error code */

        /* Set returned error string */
        rc = ISMRC_NameNotValid;
        return rc;
    }

    if (!pStatTypeString || (pStatTypeString && *pStatTypeString == '\0'))
    {
        /* TODO: Log error message and return invalid argument - may have to create an return error code */

        /* Set returned error string */
        rc = ISMRC_ArgNotValid;
        return rc;
    }

    /* Convert the text form of the StatType (pStatTypeString) into an enum */
    requestedRuleStatistic = ruleStatTypeToEnum(pStatTypeString);

    if (requestedRuleStatistic == mqcRULE_STAT_None)
    {
        /* TODO: Log error message and return invalid argument - may have to create an return error code */

        /* Set returned error string */
        rc = ISMRC_ArgNotValid;
        return rc;
    }

    /* There were probably some filter properties as well, so analyse those. */

    /* Convert the text form of the RuleType (pRuleType) to an enum. */
    requestedRuleType = ruleTypeToEnum(pRuleType);

    /* Scan through all the rules known to the bridge. For each one,  */
    /* if it corresponds to the given selection criteria then insert  */
    /* the required statistic in the sorted list of values.           */
    while (pRule != NULL)
    {
        if (ism_common_match(pRule -> pName, pRuleName))
        {
            /* We have found a rule of interest */
            if ((requestedRuleType == mqcRULE_Any) ||
                (pRule -> RuleType == requestedRuleType))
            {
                /* It is still of interest */
                pRuleQM = pRule -> pRuleQMList;

                while (pRuleQM != NULL)
                {
                    if (ism_common_match(pRuleQM -> pQM -> pName, pAssociation))
                    {
                        /* And this is an associated queue manager of interest. */
                        /* Report the required statistic.                       */

                        mqcStatistics_t
                                *pNew =
                                        (mqcStatistics_t *) mqc_malloc(sizeof(mqcStatistics_t));

                        if (pNew == NULL)
                        {
                            mqc_allocError("malloc", sizeof(mqcStatistics_t), errno);

                            /* Delete any blocks that we already obtained. */
                            freeResultList(pResults);

                            return ISMRC_AllocateError;
                        }

                        memset(pNew, 0, sizeof(mqcStatistics_t));

                        copyMqcStatistics(pNew, pRuleQM, locale);

                        mqcStatistics_t *pPrevious = NULL;
                        mqcStatistics_t *pCurrent = pResults;

                        /* Scan the existing results to find where this latest one */
                        /* should be inserted to preserve the sort order.          */

                        while (pCurrent != NULL)
                        {
                            if (statisticGT(pNew,
                                            pCurrent,
                                            requestedRuleStatistic))
                            {
                                break;
                            }
                            pPrevious = pCurrent;
                            pCurrent = pCurrent -> pNext;
                        }

                        /* At this point, pPrevious points at the block that       */
                        /* should precede the one we are inserting and pCurrent    */
                        /* points at the one that should follow it.                */

                        if (pPrevious == NULL)
                        {
                            /* Insert as a new first element in the list */
                            pNew -> pNext = pCurrent;
                            pResults = pNew;
                        }
                        else
                        {
                            pPrevious -> pNext = pNew;
                            pNew -> pNext = pCurrent;
                        }

                        if (numberOfResults == maxResults)
                        {
                            /* If the list length exceeds maxResults then delete the */
                            /* block at the end (which might be the one we just      */
                            /* added).                                               */
                            /* If pCurrent is NULL then pNew is the end of the list  */
                            /* and pPrevious precedes it so we remove what we just   */
                            /* added. Otherwise, we advance pCurrent, pNew and       */
                            /* pPrevious until we do find the end.                   */
                            while (pCurrent != NULL)
                            {
                                pPrevious = pNew;
                                pNew = pCurrent;
                                pCurrent = pCurrent -> pNext;
                            }
                            pPrevious -> pNext = NULL; /* BEAM suppression: operating on NULL */
                            mqc_free(pNew);
                        }
                        else
                        {
                            numberOfResults++;
                        }

                    }
                    pRuleQM = pRuleQM -> pNext;
                }

            }
        }
        pRule = pRule -> pNext;
    }

    if ( numberOfResults == 0 &&
    		pRuleName && *pRuleName == '*' &&
    		pAssociation && *pAssociation == '*' &&
    		pStatTypeString && !strcmp(pStatTypeString, "CommitCount") &&
    		pRuleType && !strcmp(pRuleType, "Any") ) {
        ism_common_allocBufferCopyLen(outputBuffer, "{ \"Version\":\"v1\", \"DestinationMappingRule\": [ ] }\n", 50);
        return rc;
    }


    if (numberOfResults > 0)
    {

        /* Create serialized buffer from the monitoring data we just retrieved. */
        ismJsonSerializerData iSerUserData;
        ismSerializerData iSerData;
        
        memset(&iSerUserData,0, sizeof(ismJsonSerializerData));
    	memset(&iSerData,0, sizeof(ismSerializerData));

    	iSerUserData.isExternalMonitoring = 0;
        iSerUserData.outbuf = outputBuffer;
        iSerData.serializer_userdata = &iSerUserData;

        bool first = true;
        mqcStatistics_t *pCurrent = pResults;

        ism_common_allocBufferCopyLen(iSerUserData.outbuf, "{ \"Version\":\"v1\", \"DestinationMappingRule\": [ ", 46);

        while (pCurrent != NULL)
        {
            if (!first)
            {
                ism_common_allocBufferCopyLen(iSerUserData.outbuf, ",", 1);
            }

            ism_common_serializeMonJson(XmqcStatistics_t,
                                        (void *) pCurrent,
                                        outputBuffer->buf,
                                        2500,
                                        &iSerData);

            TRACEDATA(MQC_MQAPI_TRACE,
                      "JSON Response Fragment: ",
                      0,
                      iSerUserData.outbuf->buf,
                      iSerUserData.outbuf->used,
                      iSerUserData.outbuf->used);

            first = false;
            pCurrent = pCurrent -> pNext;
        }

        ism_common_allocBufferCopyLen(iSerUserData.outbuf, " ] }", 4);

        TRACEDATA(MQC_MQAPI_TRACE,
                  "JSON Response: ",
                  0,
                  iSerUserData.outbuf->buf,
                  iSerUserData.outbuf->used,
                  iSerUserData.outbuf->used);

        /* Free the results chain */
        freeResultList(pResults);
    }
    else
    {
        //Return Error String
        rc = ISMRC_NotFound;
    }

/* MOD_EXIT: */
    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Read /proc/<pid>/statm into a buffer for parsing
///
/// @param[out]    buffer[IEMEM_MEMINFO_BUFFERSIZE]  buffer to store the contents in
/// @param[out]    bytesRead                         How many bytes were read from the file
///
/// @remark Reading files from /proc should occur in a single read (as they update between reads)
///
/// @remark Only the first IEMEM_MEMINFO_BUFFERSIZE (at most) of the file are read
//*******************************************************************************
inline int32_t mqc_readProcessMemInfo(char buffer[MQC_MEMINFO_BUFFERSIZE],
                                      int *bytesRead)
{
    int32_t rc = 0;

    char fname[64];
    sprintf(fname, "/proc/%d/statm", getpid());

    int f = open(fname, O_RDONLY);

    if (f < 0)
    {
        TRACE(MQC_NORMAL_TRACE, "Couldn't open %s, errno: %d\n", fname, errno);
        rc = ISMRC_Error;
    }
    else
    {
        int bytes_read = read(f, buffer, MQC_MEMINFO_BUFFERSIZE - 1);

        if (bytes_read < 1)
        {
            TRACE(MQC_NORMAL_TRACE, "Couldn't read from %s, errno: %d\n", fname, errno);
            rc = ISMRC_Error;
        }
        else
        {
            buffer[bytes_read] = 0;
            *bytesRead = bytes_read;
        }
        close(f);
    }
    return rc;
}

void mqc_traceProcessMemoryInfo(char *pPrefix)
{
    char processInfoBuffer[MQC_MEMINFO_BUFFERSIZE];
    uint64_t virtualMemorySize;
    uint64_t residentSetSize;
    int dummy;
    int found;
    int rc = 0;

    /* We are reporting the process virtual size as a troubleshooting */
    /* aid, so although errors are reported in the trace, we do not   */
    /* disrupt normal function in response to errors eg by restarting */
    /* mapping rules.                                                 */

    rc = mqc_readProcessMemInfo(processInfoBuffer, &dummy);
    if (rc == 0)
    {
        /* Process memory info is very simple; 7 numbers  */
        /* separated by spaces and we want the first two. */
        found = sscanf(processInfoBuffer,
                       "%lu %lu",
                       &virtualMemorySize,
                       &residentSetSize);
        if (found != 2)
        {
            TRACE(MQC_NORMAL_TRACE,"Process memory statistics not parsed\n");
        }
        else
        {
            TRACE(MQC_HIGH_TRACE,"%s: Virtual Memory Size = %lu\n", pPrefix, virtualMemorySize);
            TRACE(MQC_HIGH_TRACE,"%s: Resident Set Size = %lu\n", pPrefix, residentSetSize);
        }
    }
}

/* Provide wrapper functions for malloc and free to provide a single  */
/* location to inject logging or tracing for memory allocation        */
/* problems.                                                          */

inline void *mqc_malloc(size_t size)
{
    void *pTemp = ism_common_malloc(ISM_MEM_PROBE(ism_memory_mqcbridge_misc,8),size);

      /* if (pTemp != NULL) */
      /* { */
      /*   __sync_fetch_and_add(&mqcMQConnectivity.currentAllocatedMemory, malloc_usable_size(pTemp)); */
      /* } */
    return pTemp;
}

inline void mqc_free(void *ptr)
{
    /* __sync_fetch_and_sub(&mqcMQConnectivity.currentAllocatedMemory, malloc_usable_size(ptr)); */
    ism_common_free(ism_memory_mqcbridge_misc,ptr);
}
