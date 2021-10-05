/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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

/**
 * @brief Client interface for MessageSight SNMP subagent
 * @date 10/07/2015
 *
 * This provides interface to access $SYS/ResourceStatistics/ of IMA.
 *
 */

#define TRACE_COMP Monitoring

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include <ismutil.h>
#include <ismjson.h>
#include <ismrc.h>

#include "monitoringSnmpTrap.h"

#define MAX_JSON_ENTRIES	2000


static char *imaSnmpTopicList[imaSnmpTopic_ALL] = { NULL, //imaSnmpTopic_NONE
                                  IMA_SYS_SERVER, //imaSnmpTopic_SERVER
                                  IMA_SYS_MEMORY, //imaSnmpTopic_MEMORY
                                  IMA_SYS_STORE, //imaSnmpTopic_STORE
                                  IMA_SYS_TOPIC, //imaSnmpTopic_TOPIC
                                  IMA_SYS_ENDPOINT, //imaSnmpTopic_ENDPOINT
                                  NULL,NULL,NULL,NULL };

static imaSnmpEvent_msgHandler_t sysTopicHandler[imaSnmpTopic_ALL];


static int topicSubscribed = 0;

static int imaSnmp_getTopicType(char *topicName)
{
    int i = imaSnmpTopic_NONE;
    char *token, *nexttoken = NULL;

    if (topicName == NULL) {
    	return i;
    }

    token = strtok_r(topicName,"/",&nexttoken); //$SYS
    token = strtok_r(NULL,"/",&nexttoken); //ResourceStatistics
    token = strtok_r(NULL,"/",&nexttoken); //sub topics
    if (NULL == token) return i;

    for (i = imaSnmpTopic_NONE; i < imaSnmpTopic_ALL; i++)
    {
        if ((NULL != imaSnmpTopicList[i]) && (!strcasecmp(token,imaSnmpTopicList[i])))
        {
         // find the topic index.
            return i;
        }
    }

    TRACE(3,"invalid topic token %s \n",token);
    return imaSnmpTopic_NONE;

}

int imaSnmp_topic_handler_register(int topicType, imaSnmpEvent_msgHandler_t msgHandler)
{
    int rc = ISMRC_OK;

    if ((topicType < imaSnmpTopic_NONE) || (topicType >= imaSnmpTopic_ALL))
    {
        TRACE(2, "invalid topic type for topic handler register: %d \n", topicType);
        return ISMRC_InvalidParameter;
    }

    if (sysTopicHandler[topicType] != NULL)
    {
        TRACE(5, "the old handler for topic %d is not NULL: %p \n", topicType,
             sysTopicHandler[topicType]);
    }
    sysTopicHandler[topicType] = msgHandler;
    return rc;
}

XAPI int imaSnmp_messageArrived(char* topicName, const char *msg, int msgLen )
{
    int topicType = imaSnmp_getTopicType(topicName);
    ism_json_parse_t pobj = {0};
    ism_json_entry_t ents[MAX_JSON_ENTRIES];
    char *msgBuf;
    imaSnmpEvent_msgHandler_t msgHandler;
    int rc;


    if (msg == NULL || *msg == '\0') {
    	TRACE(3, "The message received is NULL or empty\n");
    	return true;
    }

    if ((topicType == imaSnmpTopic_NONE) || (NULL == sysTopicHandler[topicType]))
    {
        TRACE(5,"invalid topic or unregistered handler for %s \n",topicName);
        return true;
    }

    msgBuf = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,9),msgLen + 1);
    memcpy(msgBuf, msg, msgLen);
    msgBuf[msgLen] = 0;

    pobj.ent = ents;
    pobj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));
    pobj.source = (char *) msgBuf;
    pobj.src_len = msgLen;
    ism_json_parse(&pobj);

    if (pobj.rc) {
        TRACE(2,"result is not json string: %s\n", msg);
        ism_common_free(ism_memory_monitoring_misc,msgBuf);
        return true;
    }

    // invoke topic handler.
    msgHandler = sysTopicHandler[topicType];
    rc = msgHandler(&pobj);
    if (rc != ISMRC_OK)
    {
        TRACE(2,"Error in handler message for topic %s \n",topicName);
        ism_common_free(ism_memory_monitoring_misc,msgBuf);
        return true;
    }

    ism_common_free(ism_memory_monitoring_misc,msgBuf);

    return true;
}

int imaSnmp_subscribe(int topicType)
{
    int rc = ISMRC_OK;
    int topic_flag;
    char topicTemp[256];

    if ((topicType < imaSnmpTopic_NONE) || (topicType >= imaSnmpTopic_ALL))
    {
        TRACE(2, "invalid topic type for topic subscribe: %d \n", topicType);
        return ISMRC_InvalidParameter;
    }
    topic_flag = 1<<topicType;

    if (topicSubscribed & topic_flag)
    {
        TRACE(5, "topicType %d has been subscribed already. \n",topicType);
        return rc;
    }

    strcpy(topicTemp,IMA_SYS_ROOT);
    switch (topicType)
    {
    case imaSnmpTopic_SERVER:
    case imaSnmpTopic_MEMORY:
    case imaSnmpTopic_STORE:
    case imaSnmpTopic_TOPIC:
    case imaSnmpTopic_ENDPOINT:
        strcat(topicTemp, imaSnmpTopicList[topicType]);
        break;

    default:
        TRACE(2,"topic type %d is not supported for subscription. \n", topicType);
        return ISMRC_InvalidParameter;

    }

    topicSubscribed |= topic_flag;
    return ISMRC_OK;
}

int imaSnmp_unsubscribe(int topicType)
{
    int rc = ISMRC_OK;
    int topic_flag;
    char topicTemp[256];

    if ((topicType < imaSnmpTopic_NONE) || (topicType >= imaSnmpTopic_ALL))
    {
        TRACE(2, "invalid topic type for topic unsubscribe: %d \n", topicType);
        return ISMRC_InvalidParameter;
    }
    topic_flag = 1<<topicType;

    if (!(topicSubscribed & topic_flag))
    {
        TRACE(5, "topicType %d was not subscribed. \n",topicType);
        return rc;
    }


    strcpy(topicTemp,IMA_SYS_ROOT);
    switch (topicType)
    {
    case imaSnmpTopic_SERVER:
    case imaSnmpTopic_MEMORY:
    case imaSnmpTopic_STORE:
    case imaSnmpTopic_TOPIC:
    case imaSnmpTopic_ENDPOINT:
        strcat(topicTemp, imaSnmpTopicList[topicType]);
        break;

    default:
        TRACE(2,"topic type %d is not supported for unsubscription. \n", topicType);
        return ISMRC_InvalidParameter;

    }
    topicSubscribed &= ~topic_flag;
    return ISMRC_OK;
}




