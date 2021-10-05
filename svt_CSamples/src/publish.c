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
 * @file publish.c
 * 
 * The purpose of this file is to demonstrate publishing N messages using 
 * the MQTTv3 client library.
 * 
 * For more details one may refer to the MQTT client library in C 
 */


#include <mqttsample.h>

/*
 * Publish MQTTv3 messages
 *
 * @returns         MQTT_CLIENT_SUCCESS ( 0 ) if everything succeeded or
 *                  non-zero in the event of a failure.
 */
int do_publish(void){
    MQTTClient_message mqttMessage = MQTTClient_message_initializer;
    int rc=0;
    //char mymesg[2000];

    while(!g_halt){
        /* connect g_mqtt_client  */
        do_status_update();
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

    mqttMessage.payload = (char*)g_msg;
    /* the length of the payload does not need to include the nul char*/
    mqttMessage.payloadlen = strlen(g_msg);
    mqttMessage.qos = g_qos;
    mqttMessage.retained = g_retained_flag;

    while(!g_halt && (g_publish_count < g_msg_number)) {
        do_status_update();
        //sprintf(mymesg, "%d:%s", g_publish_count, g_msg);
        //mqttMessage.payload = (char*)mymesg;
        //mqttMessage.payloadlen = strlen(mymesg);
        /* Publish the message */
        
        LOG_INFO("%llu:Publishing to topic \"%s\" qos %d, message \"%s\" \n", 
           g_publish_count, g_topic, g_qos, g_msg);
    
        rc = MQTTClient_publishMessage(g_mqtt_client, (char*)g_topic, 
            &mqttMessage, &g_mqtt_token);
    
        if (rc == MQTTCLIENT_SUCCESS ) {
            /* Wait until the message has been delivered to the server */
            if (g_qos>0){
                LOG_INFO("Start WaitForCompletion\n");
                rc = MQTTClient_waitForCompletion(g_mqtt_client, g_mqtt_token, 
                        WAIT_FOR_COMPLETION_TIMEOUT);
            }
            if (rc == MQTTCLIENT_SUCCESS ) {
                g_publish_count++;
                g_appstate=STATE_PUBLISHING;
                do_rate_control(g_rate); 
            } else {
                if (do_handle_mqtt_client_errors(rc,__LINE__, __FUNCTION__,
                        "MQTTClient_publish or wfc operation" )!= 0) {
                    LOG_ERROR("Failed to deliver message, rc=%d\n",rc);
                    break;
                } // else retry operation.
            }
        } else {
            if (do_handle_mqtt_client_errors(rc,__LINE__, __FUNCTION__,
                     "Message not accepted for deliveryn" )!= 0) {
                LOG_ERROR("Failed to deliver message, rc=%d\n",rc);
                break;
            }
        }
        sleep(0);
    }

    LOG_STDOUT("Published %llu messages to topic: %s \n",g_publish_count,g_topic);

    /* Disconnect the client */
    MQTTClient_disconnect(g_mqtt_client, 0);

    LOG_INFO("Disconnected\n");

    /* Free the memory used by the client instance */
    MQTTClient_destroy(&g_mqtt_client);

    return rc;
}
