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
 * @brief MQTT Client interface for MessageSight SNMP subagent
 * @date July
 *
 * This provides MQTT interface to access $SYS/ResourceStatistics/ of IMA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include <ismutil.h>
#include <ismjson.h>
#include <ismclient.h>
#include <ismrc.h>

#include <MQTTClient.h>
#include <MQTTClientPersistence.h>

#include "imaSnmpMqttClient.h"

#define LOCAL_ADDRESS     "tcp://127.0.0.1:9089"
#define CLIENTID    "SNMPAgent"
#define TIMEOUT     10000L


static char *imaSnmpTopicList[imaSnmpTopic_ALL] = { NULL, //imaSnmpTopic_NONE
                                  IMA_SYS_SERVER, //imaSnmpTopic_SERVER
                                  IMA_SYS_MEMORY, //imaSnmpTopic_MEMORY
                                  IMA_SYS_STORE, //imaSnmpTopic_STORE
                                  IMA_SYS_TOPIC, //imaSnmpTopic_TOPIC
                                  IMA_SYS_ENDPOINT, //imaSnmpTopic_ENDPOINT
                                  NULL,NULL,NULL,NULL };

static imaSnmpEvent_msgHandler_t sysTopicHandler[imaSnmpTopic_ALL];

static MQTTClient imaSnmpMqttClient;
static MQTTClient_connectOptions imaSnmpMqtt_conn_opts=MQTTClient_connectOptions_initializer;
static int isConnected = 0;
static int topicSubscribed = 0;
static int topicSubscription = 0;

static ism_threadh_t mqttConnectionThread;
static int imaSnmpConnThread_run = 0;

static int imaSnmpMqtt_getTopicType(char *topicName)
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

int imaSnmpMqtt_topic_handler_register(int topicType, imaSnmpEvent_msgHandler_t msgHandler)
{
    int rc = ISMRC_OK;

    if ((topicType < imaSnmpTopic_NONE) || (topicType >= imaSnmpTopic_ALL))
    {
        TRACE(2, "invalid topic type for topic handler resigter: %d \n", topicType);
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

int imaSnmpMqtt_messageArrived(void* context, char* topicName, int topicLen, 
                               MQTTClient_message* message)
{
    //int msgLen = message->payloadlen;
    char *msg = (char*)message->payload;
    int topicType = imaSnmpMqtt_getTopicType(topicName);
    int buflen = 0;
    ism_json_parse_t pobj = {0};
    ism_json_entry_t ents[MAX_JSON_ENTRIES];
    char *tmpbuf;
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
    //TRACE(9,"msg received from topic %s \n",topicName);
    /* Check if mqtt message is a json string */
    buflen = message->payloadlen;
    tmpbuf = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,67),buflen + 1);
    memcpy(tmpbuf, msg, buflen);
    tmpbuf[buflen] = 0;

    pobj.ent_alloc = MAX_JSON_ENTRIES;
    pobj.ent = ents;
    pobj.source = (char *) tmpbuf;
    pobj.src_len = buflen;
    ism_json_parse(&pobj);

    if (pobj.rc) {
        TRACE(2,"result is not json string: %s\n", msg);
        ism_common_free(ism_memory_snmp_misc,tmpbuf);
        return true;
    }

    // invoke topic handler.
    msgHandler = sysTopicHandler[topicType];
    rc = msgHandler(&pobj);
    if (rc != ISMRC_OK)
    {
        TRACE(2,"Error in hanlder message for topic %s \n",topicName);
        ism_common_free(ism_memory_snmp_misc,tmpbuf);
        return true;
    }
    
    ism_common_free(ism_memory_snmp_misc,tmpbuf);

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return true;
}

void imaSnmpMqtt_connlost(void *context, char *cause)
{
    TRACE(5, "\nConnection lost\n");
    TRACE(5,"     cause: %s\n", cause);
    isConnected = 0;
}

static int mqtt_client_create()
{
    int rc = MQTTCLIENT_SUCCESS;

    TRACE(9,"MQTT Client create \n");
    rc = MQTTClient_create(&imaSnmpMqttClient, LOCAL_ADDRESS, CLIENTID, 
                           MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        TRACE(2, "failed to create Mqtt client for SNMP agent, rc = %d \n",rc);
        return rc;
    }
    TRACE(9,"MQTT Client set callback \n");
    imaSnmpMqtt_conn_opts.keepAliveInterval = 20;
    imaSnmpMqtt_conn_opts.cleansession = 1;
    rc = MQTTClient_setCallbacks(imaSnmpMqttClient,NULL, imaSnmpMqtt_connlost,
                  imaSnmpMqtt_messageArrived, NULL);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        TRACE(2, "failed to set callback for SNMP agent Mqtt client, rc = %d \n",rc);
        MQTTClient_destroy(&imaSnmpMqttClient);
    }

    TRACE(9,"MQTT Client create done \n");
    return rc;
}

static void mqtt_client_destroy()
{
    
    if (MQTTClient_isConnected(imaSnmpMqttClient))
    {
        MQTTClient_disconnect(imaSnmpMqttClient, TIMEOUT);
    }
    MQTTClient_destroy(&imaSnmpMqttClient);
    
}

static int mqtt_connect()
{
    int rc = MQTTCLIENT_SUCCESS;

    TRACE(9,"MQTT Client connect \n");
    //assume MQTTClient has been created already.
    if (MQTTClient_isConnected(imaSnmpMqttClient))
    {
        isConnected = 1;
        return rc; //already connected.
    }
    rc = MQTTClient_connect(imaSnmpMqttClient, &(imaSnmpMqtt_conn_opts));
    if (rc == MQTTCLIENT_SUCCESS)
    {
        isConnected = 1;
    }
    else TRACE(2,"failed to connect Mqtt client for SNMP agent, rc = %d \n", rc);
    //TRACE(9,"MQTT Client connect done\n");
    return rc;
}

int imaSnmpMqtt_subscribe(int topicType)
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
    topicSubscription |= topic_flag;
    if (!isConnected) 
    {
        TRACE(2, "Mqtt Client not connected for topic %d subscription\n", topicType);
        return ISMRC_NotConnected;
    }

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
    rc = MQTTClient_subscribe(imaSnmpMqttClient, topicTemp,QOS_0);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        TRACE(2,"failed to subscribe to topic %s in SNMP mqtt client, rc %d \n",topicTemp,rc);
        return rc;
    }
    topicSubscribed |= topic_flag;
    return ISMRC_OK;
}

int imaSnmpMqtt_unsubscribe(int topicType)
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
    topicSubscription &= ~topic_flag;
    if (!isConnected) 
    {
        TRACE(2, "Mqtt Client not connected for topic %d unsubscription\n", topicType);
        return ISMRC_NotConnected;
    }

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
    rc = MQTTClient_unsubscribe(imaSnmpMqttClient, topicTemp);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        TRACE(2,"failed to unsubscribe to topic %s in SNMP mqtt client, rc %d \n",topicTemp,rc);
        return rc;
    }
    topicSubscribed &= ~topic_flag;
    return ISMRC_OK;
}

static int imaSnmpMqtt_stopConnThread()
{
    int rc = 0;
    void *retVal;

    imaSnmpConnThread_run = 0;
    if (mqttConnectionThread) 
       rc =  ism_common_joinThread(mqttConnectionThread, &retVal);
    mqttConnectionThread = (ism_threadh_t)0;
    return rc;
}

static void * imaSnmpMqtt_connectionThread(void * arg, void * context, int value)
{
    unsigned int wait_time;
    unsigned int sleep_time;
    int rc = ISMRC_OK;

    imaSnmpConnThread_run = 1;
    sleep_time = CONN_SLEEP_CYCLE;
    if (value > 0) wait_time = value * CONN_CYCLE;
    else
    {
        wait_time = CONN_CYCLE;
        value = 1;
    }

    if (wait_time > MAX_CONN_CYCLE ) wait_time = MAX_CONN_CYCLE;

    while (imaSnmpConnThread_run)
    {
        if (isConnected) 
        {
            sleep(sleep_time);
            continue;
        }
        // Mqtt client not connected. try to reconnect to mqtt server.
        rc = mqtt_connect();
        TRACE(9, " mqtt connection rc = %d \n", rc);
        if (rc == MQTTCLIENT_SUCCESS)
        {
            TRACE(5, "success to reconnect SNMP Mqtt Client , %d\n", topicSubscription);
            if (topicSubscription)
            {
            //resubscribe the topics
               int i;
               int topic_flag = 1;
               topicSubscribed = 0;
               for (i = imaSnmpTopic_NONE; i < imaSnmpTopic_ALL; i++)
               {
                  if (topic_flag & topicSubscription)
                  {
                      TRACE(9, "subscribe to topic , %d\n", i);
                      rc = imaSnmpMqtt_subscribe(i);
                      if (rc)
                          TRACE(2,"failed to resubcribe topic %d for imaSnmpMqttClient \n", i);
                  }
                  topic_flag <<=1;
               }
            }
            TRACE(9, "success to reconnect SNMP Mqtt Client , %d\n", topicSubscribed);
            wait_time = value * CONN_CYCLE;
            if (wait_time > MAX_CONN_CYCLE ) wait_time = MAX_CONN_CYCLE;
            continue; 
        } 
        // fail to reconnect, retry to connect after usleep
        TRACE(5, "sleep %d us to retry mqtt connection rc = %d \n", wait_time, rc);
        //usleep(wait_time);
        ism_common_sleep(wait_time);
        wait_time *= value;
        if (wait_time > MAX_CONN_CYCLE ) wait_time = MAX_CONN_CYCLE;
    } // end of while

    return NULL;  
}

int imaSnmpMqtt_init()
{
    int rc = ISMRC_OK;

    memset(sysTopicHandler, 0, sizeof(imaSnmpEvent_msgHandler_t) * imaSnmpTopic_ALL);
    isConnected = 0;
    topicSubscribed = 0;
    topicSubscription = 0;
        
    rc = mqtt_client_create();
    if (rc != MQTTCLIENT_SUCCESS)
    {
        TRACE(2, "failed to initialize SNMP Mqtt Client \n");
        return rc;
    }
    mqtt_connect();
    /*
     * Mqtt Client connection fail should not broken the initialization process. 
    */
    /*
    if (rc != MQTTCLIENT_SUCCESS)
    {
        TRACE(2, "failed to connect SNMP Mqtt Client \n");
        return rc;
    }
    */

    // start mqttConnectionThread.
    rc = ism_common_startThread(&mqttConnectionThread,imaSnmpMqtt_connectionThread, NULL, NULL, 2,ISM_TUSAGE_TIMER,0,
           "imaSnmpMqttConnThread","mqtt client connection thread");
    if (rc)
    {
        TRACE(2, "failed to create mqtt connection thread for imaSnmpMqttClient, rc = %d \n",rc);
    }

    return rc;
}

void imaSnmpMqtt_deinit()
{
    imaSnmpMqtt_stopConnThread();
    mqtt_client_destroy();
    isConnected = 0;
    topicSubscribed = 0;
    topicSubscription = 0;
}

int imaSnmpMqtt_isConnected()
{
    return isConnected;
}

