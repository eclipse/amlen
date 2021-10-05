/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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
 * @file connect.c
 *
 * The purpose of this file is to create an connect an
 * MQTT client connection. A synchronous connection is established.
 *
 * The default initializer is used to initialize the conn_opts to 
 * {"MQTC",0,60,1,1,NULL,NULL,NULL,30,20} , and then customized with a 20 
 * second keepAliveInterval, which sets the maximum time that is allowed to
 * pass without communication between the client and the server. The reliable 
 * flag is set to false to allow up to 10 in-flight messages, and clean session
 * is set to true to prevent any state information from being stored on the
 * server.
 * 
 * For more details one may refer to the MQTT client library in C 
 *
 */

#include <mqttsample.h>


/*
 * Create and connect a synchronous MQTT client connection.
 *
 * This function handles the common setup that is needed for either a publish
 * or subscribe by creating and connecting a synchronous MQTT client connection.
 *
 * @param client     The pointer to store a handle to the MQTT client
 * @param uri        The NULL terminated string in format tcp://host:port  
 * @param clientId   The client identifer, must not be any longer than 23 chars.
 * @param persistence_type    The type of persistence to be used by the client 
 * @param persistence_context The location of the storage if necessary
 * @returns          MQTT_CLIENT_SUCCESS ( 0 ) if everything succeeded or
 *                   nonn-zero in the event of a failure.
 */
int do_connect(MQTTClient * client, 
            const char * uri, 
            const char * clientId,
            const int persistence_type, 
            const char * persistence_context,
            const int flag) {
    int rc;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_SSLOptions sslopts = MQTTClient_SSLOptions_initializer;

    g_appstate=STATE_CONNECTING;

    if (flag == MYCONNECT){
        if( *client == NULL) { /* only do for first time connection*/
            LOG_STDOUT("Attempt to connect first time with clientId %s\n",clientId);
            /* Create the client instance */
            rc = MQTTClient_create(client, (char*)uri,(char*) clientId,
                (int) persistence_type,(char*) persistence_context);
            if (rc != MQTTCLIENT_SUCCESS) {
                LOG_ERROR("Failed to create MQTT Client instance\n");
                MQTTClient_destroy(client);
                *client=NULL;
                goto out;
            }
        } else {
            LOG_STDOUT("Attempt to connect additional time with clientId %s, retry_connect %d\n",
                clientId,g_retry_connect);
        }
    } else if (flag == MYRECONNECT) {
        //10.2.12 - doing code cleanup, im not sure if this next disconnect is helpful or not
        //LOG_INFO("Disconnect client with clientId %s before attempting reconnect\n",tv->clientId);
        //MQTTClient_disconnect(tv->mqtt_client, 0);
        LOG_STDOUT("Attempt to reconnect with clientId %s retry_connect %d\n",clientId, g_retry_connect);
    } else {
        HALT_ERROR("Internal code bug - invalid flag : %d\n", flag);
    }

    /* Create the connection options */
    conn_opts.keepAliveInterval = KEEPALIVEINTERVAL_SECONDS;
    //conn_opts.keepAliveInterval = 20;
    conn_opts.reliable = RELIABLE_FLAG_FALSE;
    conn_opts.cleansession = g_cleansession;
    conn_opts.connectTimeout= 36000;// should retry if not connected...?
    //conn_opts.connectTimeout= 1;// should retry if not connected...?
    //conn_opts.retryInterval= 1;// should retry if not connected...?

    /* set user name if specified */
    if (g_user[0] != '\0'){
        LOG_INFO("Setting user\n");
        conn_opts.username=g_user;
    }

    /* set user password if specified */
    if (g_password[0] != '\0'){
        LOG_INFO("Setting password\n");
        conn_opts.password=g_password;
    }

    /* Setup SSL */
    conn_opts.ssl=&sslopts;
    if (g_keyStore) {
        LOG_INFO("Setting keyStore to %s\n", g_keyStore);
        conn_opts.ssl->keyStore=g_keyStore;
    }
    if (g_trustStore) {
        LOG_INFO("Setting trustStore to %s\n", g_trustStore);
        conn_opts.ssl->trustStore=g_trustStore;
    }
    if (g_privateKey) {
        LOG_INFO("Setting privateKey to %s\n", g_privateKey);
        conn_opts.ssl->privateKey=g_privateKey;
    }
    if (g_privateKeyPassword) {
        LOG_INFO("Setting privateKeyPassword to %s\n", g_privateKeyPassword);
        conn_opts.ssl->privateKeyPassword=g_privateKeyPassword;
    }
    if (g_enabledCipherSuites) {
        LOG_INFO("Setting enabledCipherSuites to %s\n", g_enabledCipherSuites);
        conn_opts.ssl->enabledCipherSuites=g_enabledCipherSuites;
    }
    conn_opts.ssl->enableServerCertAuth=g_enableServerCertAuth;


    /* Connect the client */
    while (!g_halt && (g_retry_connect>0)) {
        if(g_retry_connect == 60) {
             LOG_WARNING("Connect retries decreased to alert level: %d\n", g_retry_connect);
        }
        if(g_retry_connect == 30) {
             LOG_WARNING("Connect retries decreased to warning level: %d\n", g_retry_connect);
        }
        if(g_retry_connect < 5) {
             LOG_WARNING("Connect retries decreased to critical level: %d\n", g_retry_connect);
        }

        rc = MQTTClient_connect(*client, &conn_opts);
        if (rc != MQTTCLIENT_SUCCESS) {
            if (do_handle_mqtt_client_errors(rc,__LINE__, __FUNCTION__,
                        "MQTTClient_connect operation" )!= 0) {
                LOG_ERROR("Failed to connect, rc=%d\n", rc);
                MQTTClient_destroy(client);
                break;
            } // else retry operation
            g_retry_connect--;
            break;
        } else {
            break; // successfully connected.
        }
    }
out:
    if (rc == MQTTCLIENT_SUCCESS) {
        g_appstate=STATE_CONNECTED;
        g_retry_connect=240; /* refresh to full number of retries */
        if (flag == MYCONNECT) { /* only do for first time connection*/
            LOG_STDOUT("Connected to %s , clientId : %s - Ready\n",uri,clientId);
        } else if (flag == MYRECONNECT) {
            LOG_STDOUT("Reconnected to %s , clientId : %s - Ready\n",uri, clientId);
        } else {
            HALT_ERROR("Internal code bug - invalid flag : %d\n", flag);
        }
    } else {
        g_appstate=STATE_DISCONNECTED;
        //LOG_ERROR("Failed to connect, and ran out of retries rc=%d retry_connect=%d\n", rc, tv->retry_connect);
        LOG_STDOUT("Disconnect is %d Client connection attempt failed with clientId %s retry_connect %d state:%d\n",
            STATE_DISCONNECTED,clientId, g_retry_connect, g_appstate);
    }

    return rc;
}

/*
 * Reconnect a synchronous MQTT client connection.
 *
 * This function will attempt to reconnect an MQTT client connection
 * for up to g_retry_connect retries, before returning a failure. 
 * The caller should continue to to try again, as needed, 
 * until a failure is returned.
 *
 * @returns         non-zero in the event of a failure.
 *                 
 */
int do_reconnect(void){
    int rc=0;
    if(g_retry_connect>0) {
        if (g_appstate!=STATE_CONNECTING){
            LOG_WARNING("Attempting to reconnect client\n");
            /* reconnect client  */
            rc = do_connect(&g_mqtt_client, g_uri, g_clientId, g_persistence_type,
                     g_persistence_context, MYRECONNECT);
            if (rc != MQTTCLIENT_SUCCESS) {
                LOG_WARNING("Failed to reconnect MQTT Client with rc=%d\n",rc);
                rc=0;  /* user should try again.*/
            }
        } /* else already trying to connect. don't bother it*/
    } else {
        LOG_ERROR("Failed to reconnect MMQTT Client (Out of Retries)\n");
        rc=__LINE__; /* application should fail now. */
    }
    return rc;
}

  
