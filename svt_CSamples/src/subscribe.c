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

/*
 * @file subscribe.c
 * 
 * The purpose of this file is to demonstrate subscribing to N messages using 
 * the MQTTv3 client library.
 * 
 * For more details one may refer to the MQTT client library in C 
 */

#include <mqttsample.h>

/*
 * Subscribes to a topic and blocks until Ctrl-C is pressed,
 * the connection is lost, or the total number of planned messages
 * has been sent.
 *
 * The sample demonstrates synchronous subscription, which
 * does not use callbacks.
 *
 * @returns         MQTT_CLIENT_SUCCESS ( 0 ) if everything succeeded or
 *                  non-zero in the event of a failure.
 */
int do_subscribe(void){
    int rc=0;
    MQTTClient_message* message = NULL;
    char* receivedTopicName = NULL;
    long receiveTimeout = 100l;
    int topicLen=0;

    while(!g_halt){
        do_status_update();
        /* connect g_mqtt_client  */
        rc = do_connect(&g_mqtt_client, g_uri, g_clientId, g_persistence_type, 
            g_persistence_context, MYCONNECT);
        if (rc != MQTTCLIENT_SUCCESS) {
            if (do_handle_mqtt_client_errors(rc,__LINE__, __FUNCTION__,
                        "MQTTClient connect operation" )!= 0) {
                LOG_ERROR("Failed to connect MQTT Client with rc=%d\n",rc);
                return rc;
            }
        } else {
            break; /* successfull connect*/
        }
    }

    LOG_INFO("Subscribing to topic \"%s\" qos %d\n", g_topic, g_qos);

    while(!g_halt){
        do_status_update();
        /* Subscribe to the g_topic */
        rc = MQTTClient_subscribe(g_mqtt_client,(char*) g_topic, g_qos);
        if (rc != MQTTCLIENT_SUCCESS) {
            if (do_handle_mqtt_client_errors(rc,__LINE__, __FUNCTION__,
                        "MQTTClient_subscribe operation" )!= 0) {
                LOG_ERROR("Failed to subscribe, rc=%d\n", rc);
                g_halt=1; // discontinue further processing 
                break;
            } // else retry the operation.
        } else {
            g_appstate=STATE_SUBSCRIBED;
            LOG_STDOUT("Subscribed - Ready\n");
            break;
        }
    }

    while (!g_halt && (g_subscribe_count<g_msg_number)) {
        do_status_update();
        receivedTopicName = NULL;
        topicLen=0;

        /* Check to see if a message is available */
        rc = MQTTClient_receive(g_mqtt_client, &receivedTopicName, &topicLen, 
                          &message, receiveTimeout);

        if (rc == MQTTCLIENT_SUCCESS && message != NULL) {
            /* A message has been received */
            g_appstate=STATE_RECEIVING;
            g_subscribe_count++;
            do_rate_control(g_rate);
            LOG_INFO("Message %llu " \
                   "received on topic '%s': " \
                   "%.*s\n",
               g_subscribe_count,
               receivedTopicName, 
               message->payloadlen, (char*)message->payload);
            MQTTClient_freeMessage(&message);
            MQTTClient_free(receivedTopicName);
        } else if (rc != MQTTCLIENT_SUCCESS) {
            if (do_handle_mqtt_client_errors(rc,__LINE__, __FUNCTION__,
                        "MQTTClient_receive operation" )!= 0) {
                LOG_ERROR("Failed to receive message, rc=%d\n",rc);
                break; 
            }
        }
        sleep(0);
    }

    LOG_STDOUT("Received %llu messages.\n",g_subscribe_count);

    /* Disconnect the g_mqtt_client */
    MQTTClient_disconnect(g_mqtt_client, 0);

    LOG_INFO("Disconnected\n");

    /* Free the memory used by the g_mqtt_client instance */
    MQTTClient_destroy(&g_mqtt_client);

    return rc;
}
