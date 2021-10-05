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

#if 0

TODO - to prevent stale connections, i need a way to tell all clients to disconnect and exit instead of just killing them.




Console> imaserver stat

ActiveConnections = 560679 

TotalConnections = 2065146 

MsgRead = 66 

MsgWrite = 3657954 

BytesRead = 96490581 

BytesWrite = 140380953 

BadConnCount = 0 

TotalEndpoints = 1 

Console> 

#endif


/*
 * @file connect_array.c
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

#include <mqttsample_array.h>


/*
 * Create and connect a synchronous MQTT client connection.
 *
 * This function handles the common setup that is needed for either a publish
 * or subscribe by creating and connecting a synchronous MQTT client connection.
 *
 * @param tv         All of the data neeaded for this client
 * @returns          MQTT_CLIENT_SUCCESS ( 0 ) if everything succeeded or
 *                   nonn-zero in the event of a failure.
 */
int do_connect(clientVariables *tv, int flag){
    int rc=__LINE__;
    char *user=NULL;
    char *clientcert=NULL;
    char *clientkey=NULL;
    int recover_connections=0;
    unsigned long long  startTime=0;
    unsigned long long  endTime=0;
    unsigned long long  totalTime=0;
    
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_SSLOptions sslopts = MQTTClient_SSLOptions_initializer;
    MQTTClient_willOptions willopts = MQTTClient_willOptions_initializer;


    if (av.destroyClientBeforeConnect==1){
        CLIENT_LOG_INFO(tv,"User Override - Destroying client state before connecting\n");
        do_disconnect_and_destroy_client(tv);
    }


    tv->clientstate=STATE_CONNECTING;
    if (av.haConnect>=1 ){
        if (av.haHint>=1){
            av.haConnect=av.haHint;
        }
    }

    if( (tv->primary_mqtt_client == NULL) && (av.haConnect<=1)) { /* only do for first time connection*/
        if (strlen(tv->clientId) <= 0){
            HALT_ERROR("Internal code bug - invalid clientId %s\n", tv->clientId);
        }
        CLIENT_LOG_INFO(tv,"Creating Primary MQTT Client with clientId %s\n",tv->clientId);
        rc = wrap_MQTTClient_create(&tv->primary_mqtt_client, (char*)av.uri,(char*) tv->clientId,
            (int) av.persistence_type,(char*) av.persistence_context);
        if (rc != MQTTCLIENT_SUCCESS) {
            CLIENT_LOG_ERROR(tv,"Failed to create primary MQTT Client instance\n");
            wrap_MQTTClient_destroy(&tv->primary_mqtt_client);
            tv->primary_mqtt_client=NULL;
            goto out;
        }
    }

    if( (tv->backup_mqtt_client == NULL) && (av.haConnect==2)) { /* only do for first time connection*/
        CLIENT_LOG_INFO(tv,"Creating Backup MQTT Client for HighAvailability with clientId %s\n",tv->clientId);
        if (strlen(tv->clientId) <= 0){
            HALT_ERROR("Internal code bug - invalid clientId %s\n", tv->clientId);
        }
        rc = wrap_MQTTClient_create(&tv->backup_mqtt_client, (char*)av.haURI,(char*) tv->clientId,
            (int) av.persistence_type,(char*) av.persistence_context);
        if (rc != MQTTCLIENT_SUCCESS) {
            CLIENT_LOG_ERROR(tv,"Failed to create backup MQTT Client instance\n");
            wrap_MQTTClient_destroy(&tv->backup_mqtt_client);
            tv->backup_mqtt_client=NULL;
            goto out;
        }
    }

    if (av.haConnect>=1 ){
        if (av.haConnect == 1 )  {
            tv->curURI=av.uri;
            if (tv->mqtt_client != tv->primary_mqtt_client ) {
                tv->mqtt_client=tv->primary_mqtt_client;
                CLIENT_LOG_STDOUT(tv,"Using Primary HA Client @ %s\n", av.uri);
            }
            av.haConnect=2; // next time use other client
        }else if (av.haConnect == 2 )  {
            tv->curURI=av.haURI;
            if (tv->mqtt_client != tv->backup_mqtt_client ) {
                tv->mqtt_client=tv->backup_mqtt_client;
                CLIENT_LOG_STDOUT(tv,"Using Backup HA Client @ %s\n", av.haURI);
            }
            av.haConnect=1; // next time use other client
        }else{ 
            HALT_ERROR("Internal code bug - invalid av.haConnect value : %d\n", av.haConnect);
        }
    } else {
        tv->curURI=av.uri;
        if (tv->mqtt_client != tv->primary_mqtt_client ) {
            CLIENT_LOG_STDOUT(tv,"Using Primary Client @ %s\n", av.uri);
            tv->mqtt_client=tv->primary_mqtt_client;
        }
    }
    if (tv->mqtt_client == NULL){
        HALT_ERROR("Internal code bug - tv->mqtt_client == NULL \n");
    }

    if (flag == MYCONNECT){
        if (av.noDisconnect==0){
        //10.2.12 - doing code cleanup, im not sure if this next disconnect is helpful or not
            CLIENT_LOG_INFO(tv,"Disconnect client with clientId %s before attempting connect\n",tv->clientId);
            wrap_MQTTClient_disconnect(tv->mqtt_client, 0);
        } else {
            CLIENT_LOG_INFO(tv,"NOT Disconnected during reconnect first attempt (test parameter noDisconnect\n");
        }
        if (av.reconnectWait > 0 && tv->retry_connect < av.retry_connect){
            if (tv->array_id == 0 ){ // only wait on the 0th client
                CLIENT_LOG_INFO(tv,"Sleep %d seconds before attempting to connect.\n",av.reconnectWait);
                do_safe_mqtt_client_sleep(tv,av.reconnectWait);
            }
        }
    } else if (flag == MYRECONNECT) {
        if (av.noDisconnect==0){
            //10.2.12 - doing code cleanup, im not sure if this next disconnect is helpful or not
            CLIENT_LOG_INFO(tv,"Disconnect client with clientId %s before attempting reconnect\n",tv->clientId);
            wrap_MQTTClient_disconnect(tv->mqtt_client, 0);
        } else {
            CLIENT_LOG_INFO(tv,"NOT Disconnected during reconnect (test parameter noDisconnect\n");
        }
        if (av.reconnectWait > 0 ){
            if (tv->array_id == 0 ){ // only wait on the 0th client
                CLIENT_LOG_INFO(tv,"Sleep %d seconds before reconnecting\n",av.reconnectWait);
                do_safe_mqtt_client_sleep(tv,av.reconnectWait);
            }
        } 
        CLIENT_LOG_STDOUT(tv,"Attempt to reconnect with clientId %s retry_connect %d\n",tv->clientId, tv->retry_connect);
    } else {
        HALT_ERROR("Internal code bug - invalid flag : %d\n", flag);
    }

    if (av.keepAliveInterval    >=    0 ){
        CLIENT_LOG_INFO(tv,"Using custom keep alive interval : %d\n", av.keepAliveInterval);
        conn_opts.keepAliveInterval = av.keepAliveInterval;
    } else {
        /* Create the connection options */
        conn_opts.keepAliveInterval = KEEPALIVEINTERVAL_SECONDS;
    }

    if (av.reliable    >=    0 ){
        CLIENT_LOG_INFO(tv,"Using custom reliable flag: %d\n", av.reliable);
        conn_opts.reliable = av.reliable;
    } else {
        conn_opts.reliable = RELIABLE_FLAG_FALSE;
    }

    conn_opts.cleansession = av.cleansession;

    //conn_opts.connectTimeout= 240;// should retry if not connected...?
    conn_opts.connectTimeout= 36000;// should retry if not connected...?
    if (av.connectTimeout >= 0 ){
        CLIENT_LOG_INFO(tv,"Overriding the default connectTimeout with %d\n", av.connectTimeout);
        conn_opts.connectTimeout=av.connectTimeout;
    }
    //conn_opts.connectTimeout= 1;// should retry if not connected...?
    //conn_opts.connectTimeout= 1;// should retry if not connected...?
    //conn_opts.retryInterval= 1;// should retry if not connected...?


    /* set user name if specified */
    if (av.user[0] != '\0'){
        if (av.userIncrementer >= 0 ){
            user=(char*)malloc(32);
            sprintf(user, "%s%07d",av.user,av.userIncrementer+tv->array_id);
        } 
        if (user){  
            CLIENT_LOG_INFO(tv,"Setting user w/userIncrementer %d : %s\n", av.userIncrementer , user );
            conn_opts.username=user;
        } else {
            CLIENT_LOG_INFO(tv,"Setting user: av.user : %s\n",av.user );
            conn_opts.username=av.user;
        }
    }

    /* set user password if specified */
    if (av.userLtpaFile[0]!='\0'){
        if (tv->ltpaToken==NULL){
            if (get_next_ltpa_token(tv) == NULL ){
                HALT_ERROR("Internal code bug - could not get ltpa token\n");
            }
        }
        CLIENT_LOG_INFO(tv,"Setting password: tv->ltpaToken : %s\n",tv->ltpaToken);
        conn_opts.password=tv->ltpaToken;
    }else if (av.password[0] != '\0'){
        CLIENT_LOG_INFO(tv,"Setting password: av.password : %s\n",av.password );
        conn_opts.password=av.password;
    }

    /* Setup SSL */
    conn_opts.ssl=&sslopts;
    if (av.trustStore) {
        CLIENT_LOG_INFO(tv,"Setting trustStore to %s\n", av.trustStore);
        conn_opts.ssl->trustStore=av.trustStore;
    }
    if (av.dynamicClientCert[0]!='\0'){
        if (av.dccIncrementer >= 0 ){
            clientcert=(char*)malloc(32+strlen(av.dynamicClientCert));
            sprintf(clientcert, "%s/%d/CN%07d.pem",av.dynamicClientCert,
                ((av.dccIncrementer+tv->array_id)/1000)*1000,
                av.dccIncrementer+tv->array_id);
            clientkey=(char*)malloc(32+strlen(av.dynamicClientCert));
            sprintf(clientkey, "%s/%d/CN%07d.key",av.dynamicClientCert,
                ((av.dccIncrementer+tv->array_id)/1000)*1000,
                av.dccIncrementer+tv->array_id);
            CLIENT_LOG_INFO(tv,"Dynamically setting keyStore to %s\n", clientcert);
            conn_opts.ssl->keyStore=clientcert;
            CLIENT_LOG_INFO(tv,"Dynamically setting privateKey to %s\n", clientkey);
            conn_opts.ssl->privateKey=clientkey;
            //CLIENT_LOG_INFO(tv,"topicCNPrepend is %s done.\n", tv->topicCNPrepend);
            //CLIENT_LOG_INFO(tv,"topicUSPrepend is %s done.\n", tv->topicUSPrepend);
            if ( av.prependCN2Topic != 0){
                sprintf(tv->topicCNPrepend, "/CN%07d", av.dccIncrementer+tv->array_id);
                CLIENT_LOG_INFO(tv,"Dynamically setting tv->topicCNPrepend to %s\n", tv->topicCNPrepend);
            } 
            //CLIENT_LOG_INFO(tv,"topicCNPrepend is %s done.\n", tv->topicCNPrepend);
            //CLIENT_LOG_INFO(tv,"topicUSPrepend is %s done.\n", tv->topicUSPrepend);
        } else {
            HALT_ERROR("User Error: Invalid usage, must specify av.dccIncrementer if using av.dynamicClientCertficate\n");
        }
    } else{
        if (av.keyStore) {
            CLIENT_LOG_INFO(tv,"Setting keyStore to %s\n", av.keyStore);
            conn_opts.ssl->keyStore=av.keyStore;
        }
        if (av.privateKey) {
            CLIENT_LOG_INFO(tv,"Setting privateKey to %s\n", av.privateKey);
            conn_opts.ssl->privateKey=av.privateKey;
        }
    }
    if (av.privateKeyPassword) {
        CLIENT_LOG_INFO(tv,"Setting privateKeyPassword to %s\n", av.privateKeyPassword);
        conn_opts.ssl->privateKeyPassword=av.privateKeyPassword;
    }
    if (av.enabledCipherSuites) {
        CLIENT_LOG_INFO(tv,"Setting enabledCipherSuites to %s\n", av.enabledCipherSuites);
        conn_opts.ssl->enabledCipherSuites=av.enabledCipherSuites;
    }
    conn_opts.ssl->enableServerCertAuth=av.enableServerCertAuth;

    if (av.willTopic) {
        CLIENT_LOG_INFO(tv,"Adding will topic to conn opts: %s\n", av.willTopic);
        willopts.topicName=av.willTopic;
        willopts.message=av.willMessage;
        willopts.retained=av.willRetained;
        CLIENT_LOG_INFO(tv,"Adding will retained value: %d to conn opts\n", av.willRetained);
        willopts.qos=av.willQos;
        conn_opts.will=&willopts;
    } 

    rc=__LINE__; // failed to connect.

    /* Connect the client */
    while (!av.halt && (tv->retry_connect>0)) {
        if(tv->retry_connect == 60) {
             CLIENT_LOG_WARNING(tv,"Connect retries decreased to alert level: %d\n", tv->retry_connect);
        }
        if(tv->retry_connect == 30) {
             CLIENT_LOG_WARNING(tv,"Connect retries decreased to warning level: %d\n", tv->retry_connect);
        }
        if(tv->retry_connect < 5) {
             CLIENT_LOG_WARNING(tv,"Connect retries decreased to critical level: %d\n", tv->retry_connect);
        }

        CLIENT_LOG_INFO(tv,"Attempt to connect: clientId: %s retry_connect: %d connectionCounter: %d connectionFailure: %d\n",
                tv->clientId, tv->retry_connect,tv->connectionCounter,tv->connectionFailure);
        startTime=getHRTimer();
        rc = wrap_MQTTClient_connect(tv->mqtt_client, &conn_opts);
        endTime=getHRTimer();
        av.connectionAttempts++; 
        totalTime=endTime-startTime;
        if(av.connectionMinTime > totalTime) { av.connectionMinTime=totalTime; }
        if(av.connectionMaxTime < totalTime) { av.connectionMaxTime=totalTime; }
        av.connectionSumTime+=totalTime;
        CLIENT_LOG_STDOUT(tv,"MQTTClient_connect rc: %d seconds: %f connectionCounter: %d connectionFailure: %d\n",
                 rc, totalTime/MICROS_PER_SECOND , tv->connectionCounter,tv->connectionFailure);
        if ((totalTime/MICROS_PER_SECOND)<av.connectRate) {
            CLIENT_LOG_INFO(tv,"Start Sleep because connection rate is too fast\n");
            do_safe_mqtt_client_usleep(tv,(av.connectRate-(totalTime/MICROS_PER_SECOND))*MICROS_PER_SECOND);
            CLIENT_LOG_INFO(tv,"End Sleep because connection rate is too fast\n");
        }
        
        if (rc != MQTTCLIENT_SUCCESS) {
            if (do_handle_mqtt_client_errors(tv,rc,__LINE__, __FUNCTION__,
                        "MQTTClient_connect operation" )!= 0) {
                CLIENT_LOG_ERROR(tv,"Failed to connect, rc=%d\n", rc);
                // TODO: Removed on 9.19.2012... not sure if this is a good idea:
                // having segfault problems after errors.
                //MQTTClient_destroy(&tv->mqtt_client);
            } 
            tv->retry_connect--;
            break;
        } else {
            break; // successfully connected.
        }
    }

out:
    if (user){ free(user);}
    if (clientcert){ free(clientcert);}
    if (clientkey){ free(clientkey);}
    if (rc == MQTTCLIENT_SUCCESS) {
        
        if (av.warningWaitDynamic==1 ){
            av.warningWait=0; // dont wait any more after a warning until next connect failure.
        }
        if (av.haConnect>=1 ){
            // haHint makes the client run faster when the -z option is used with multiple client,
            // it allows it so that each client doesn't need to discover which appliance is the active
            // appliance.
            if (av.haConnect==1){
                av.haHint=2; // if haConnect is now == 1, then it means success happened with 2.
            } else if (av.haConnect==2){
                av.haHint=1; // if haConnect is now == 2, then it means success happened with 1.
            } else {
                av.haHint=0;
            }
        }
        tv->connectionCounter++;
        if ((tv->lastActionURI != NULL       ) &&
            (tv->lastActionURI != tv->curURI) ) {
            CLIENT_LOG_STDOUT(tv,"HA failover detected. tv->lastActionURI:" \
                    " %s != tv->curURI %s - recover and reconnect all clients\n",
                    tv->lastActionURI, tv->curURI);
            recover_connections=1;
        }

        tv->lastActionURI=NULL; // this is a new connection no actions have been performed yet
        av.connectionCounter++;
        tv->clientstate=STATE_CONNECTED;
        tv->retry_connect=av.retry_connect; /* refresh to full number of retries */
        if (flag == MYCONNECT) { /* only do for first time connection*/
            CLIENT_LOG_STDOUT(tv,"array_id:%d: Connected to %s , clientId : %s - Ready\n",tv->array_id, tv->curURI, tv->clientId);
        } else if (flag == MYRECONNECT) {
            CLIENT_LOG_STDOUT(tv,"array_id:%d: Reconnected to %s , clientId : %s - Ready\n",tv->array_id, tv->curURI, tv->clientId);
        } else {
            HALT_ERROR("Internal code bug - invalid flag : %d\n", flag);
        }

        if (av.unsubscribeAfterConn == 1){
            // -- WARNING - this could get very messy with all but the most basic cases
            // -- WARNING - Not sure if this will work with flags like -z , topicChangeTest, Multi etc...
            // -- WARNING - Do not expect this to work except with very basic use case.
            do_set_client_topic(tv, PUBLISH); 
            rc=wrap_MQTTClient_unsubscribe(tv->mqtt_client,tv->topic);
            CLIENT_LOG_STDOUT(tv,"Unsubscribe After Connect rc:%d.\n",rc);
        }

        // --RTC defect 28720 - do I need to resubscribe when i reconnect? ------
        if (av.subscribeOnReconnect>0 || av.subscribeOnConnect>0){
            if ( (av.subscribeOnReconnect>0 && av.tv->rate_s.nmsgs>0) ||
            	 (av.subscribeOnConnect>0) )  { 
                if (strcmp(av.action, SUBSCRIBE) == 0){
                // retry until successful
                    while(!av.halt){
                        rc=do_single_subsription(tv);
                        if (rc == MQTTCLIENT_SUCCESS) {
                            tv->clientstate=STATE_SUBSCRIBED; 
                            break; 
                        }
                    }
                }//else it is not really valid to specify this flag for publishers, but i wont log an error for it.
            }
        }
    #if 0 
        // - 10.30.2013 - shouldn't this be moved to after do_connect_array loop has finished so fan-in pattern works better?
        // --RTC defect 28720, - do I need to wait to publish after I reconnect? ------
        if (av.publishDelayOnConnect>0){ // USER wants a delay after reconnections (mostly for HA testing - RTC 28720)
            if (av.tv->rate_s.nmsgs > 0) { // dont delay on first connection, only on reconnects
                if (strncmp(av.action, PUBLISH,7) == 0){
                    CLIENT_LOG_INFO(tv,"------ WAITING %d seconds to start publishing again. TODO: This will " \
                                    " NOT work well with the -z option and -a publish -----\n", av.publishDelayOnConnect);
                    do_safe_mqtt_client_sleep(tv,av.publishDelayOnConnect); 
                    CLIENT_LOG_INFO(tv,"------ DONE. Waited %d seconds before starting to publish again -----\n", av.publishDelayOnConnect);
                }
            }
        }
    #endif
        if (recover_connections==1){
            CLIENT_LOG_STDOUT(tv,"HA failover start recovery \n");
            rc = do_check_array_connections(av.tv_array, tv->curURI);
            CLIENT_LOG_STDOUT(tv,"HA failover recovery complete - rc : %d\n",rc);
        }
    } else {
        if (av.warningWaitDynamic==1 ){
            av.warningWait=1; // wait 1 second on each warning until next connect success.
        }
        tv->connectionFailure++;
        av.connectionFailure++;
        tv->clientstate=STATE_DISCONNECTED;
        CLIENT_LOG_WARNING(tv,"Connection Failed: rc: %d for clientId %s retry_connect %d state:%d\n",
            rc ,tv->clientId, tv->retry_connect, tv->clientstate);

        // Needed until RTC 24097 is fixed...
        if (av.haConnect>=1 ){
            // haHint makes the client run faster when the -z option is used with multiple client,
            // it allows it so that each client doesn't need to discover which appliance is the active
            // appliance.
            if (av.haConnect==1){
                av.haHint=1; // if haConnect is now == 1, then it means failure happened with 2.
            } else if (av.haConnect==2){
                av.haHint=2; // if haConnect is now == 2, then it means failure happened with 1.
            } else {
                av.haHint=0;
            }
        }

        //if (av.haConnect>=1 || av.destroyClientOnAllConnectFails >=1){
        if (av.destroyClientOnAllConnectFails >=1){
            //if (av.haConnect>=1 ) {
                //CLIENT_LOG_INFO(tv,"In High Availablity mode, additional cleanup is required RTC 24097\n" );
            //}
            if (av.destroyClientOnAllConnectFails >=1){
                CLIENT_LOG_INFO(tv,"USER Specified additional cleanup for connect failures. Disconnect and destroy clients. Warning: RTC 30235 memory may leak.\n" );
            }
            if (tv->primary_mqtt_client) {
                if (av.noDisconnect==0){
                    CLIENT_LOG_INFO(tv,"Disconnect Primary client, before it can be used again on next try.\n ");
                    wrap_MQTTClient_disconnect(tv->primary_mqtt_client, 0);
                } else {
                    CLIENT_LOG_INFO(tv,"NOT Disconnect Primary client, before it can be used again on next try.\n ");
                }
                wrap_MQTTClient_destroy(&tv->primary_mqtt_client);
                CLIENT_LOG_INFO(tv,"Destroyed Primary client\n ");
                tv->primary_mqtt_client=NULL;
            }
            if (tv->backup_mqtt_client) {
                if (av.noDisconnect==0){
                    CLIENT_LOG_INFO(tv,"Disconnect Backup client, before it can be used again on next try.\n ");
                    wrap_MQTTClient_disconnect(tv->backup_mqtt_client, 0);
                } else {
                    CLIENT_LOG_INFO(tv,"NOT Disconnect Backup client, before it can be used again on next try.\n ");
                }
                wrap_MQTTClient_destroy(&tv->backup_mqtt_client);
                CLIENT_LOG_INFO(tv,"Destroyed Backup client\n ");
                tv->backup_mqtt_client=NULL;
            }
            tv->mqtt_client=NULL;
        }

        if (tv->retry_connect<=0){
            CLIENT_LOG_ERROR(tv,"Out of Retries: Failed to Connect MMQTT Client (Out of Retries)\n");
        }
    }

    return rc;
} 


int do_check_array_connections(clientVariables** tv, char*curURI){
    int rc=0, reconnect=0;
    LOG_INFO("Check Array connections\n");
    MY_ARRAY_INIT_VARIABLES;
    MY_ARRAY_CHECK_ERROR_LOOP_START
    if ((tv[i]->lastActionURI != NULL       ) &&
        (tv[i]->lastActionURI != curURI) ) {
        if ( tv[i]->clientstate < STATE_COMPLETE) {
            CLIENT_LOG_INFO(tv[i],"Setting client state to failover state. tv[i]->lastActionURI:" \
                        " %s != curURI %s - client needs to be connected to the new server after failover.\n",
                        tv[i]->lastActionURI, curURI);
            tv[i]->clientstate = STATE_FAILOVER;
            tv[i]->lastActionURI=NULL;
            reconnect++;
        }
    }else if (tv[i]->mqtt_client!=NULL){
        rc=MQTTClient_isConnected(tv[i]->mqtt_client);
        if ( tv[i]->clientstate < STATE_COMPLETE) {
            if (rc==0) {
                CLIENT_LOG_INFO(tv[i],"Client is not connected: MQTTClient_isConnected:%d \n",rc);
                tv[i]->clientstate = STATE_DISCONNECTED;
                reconnect++;
            } else {
                CLIENT_LOG_INFO(tv[i],"No action( A ): Client is connected: MQTTClient_isConnected:%d \n",rc);
            }
        } else  {
            CLIENT_LOG_INFO(tv[i],"No action( B ): Client is connected: MQTTClient_isConnected:%d \n",rc);
        }
    }
    MY_ARRAY_CHECK_ERROR_LOOP_END

    rc=0;
    if (reconnect>0){
        LOG_INFO("Reconnect Array connections: %d detected as potentially being disconnected\n",reconnect);
        // connect client  
        rc = do_connect_array(tv);
        if (rc != 0){
            LOG_ERROR("Failed to connect array of clients\n");
        } else {
            LOG_INFO("Successfully reconnected the array of clients\n");
        }
    }
    // - 12.10.2013 - I think now that this is the proper spot for this code... still not sure it will be better in all cases, but
    //              - currently working with a single publisher and subscriber, the publisher did not execute the delay in the 
    //              - previous spot, which was after the do_connect_array because for a single client the above if condition
    //              - does not call do_connect_array because the single client has already been reconencted.
    // - 10.30.2013 - shouldn't this be moved to after do_connect_array loop has finished so fan-in pattern works better?
    // --RTC defect 28720, - do I need to wait to publish after I reconnect? ------
    if (av.publishDelayOnConnect>0){ // USER wants a delay after reconnections (mostly for HA testing - RTC 28720)
        if (av.tv->rate_s.nmsgs > 0) { // dont delay on first connection, only on reconnects
            if (strncmp(av.action, PUBLISH,7) == 0){
                LOG_INFO("------ WAITING %d seconds to start publishing again. \n", av.publishDelayOnConnect);
                do_safe_mqtt_client_sleep(av.tv_array[0],av.publishDelayOnConnect); 
                LOG_INFO("------ DONE. Waited %d seconds before starting to publish again -----\n", av.publishDelayOnConnect);
            }
        }
    }
    return rc;
    
}

int do_connect_array(clientVariables** tv){
    int rc=__LINE__;
    MY_ARRAY_INIT_VARIABLES;
    while(!av.halt){
        LOG_INFO("Start connect array pass, total clients is %d\n", tv[0]->total_clients);
        MY_ARRAY_CHECK_CLIENT_STATE_GREATER_THAN_OR_EQ(STATE_CONNECTED,"Connected");
        MY_ARRAY_CHECK_ERROR_LOOP_START
        do_status_update(tv[0],0);
        //LOG_INFO("start of loop i is %d\n", i);
        //LOG_INFO("Start connect array pass, total clients is %d i is :%d\n", tv[0]->total_clients,i);
        if(tv[i]->clientstate >= STATE_CONNECTED) {
            LOG_INFO("%d: Continue client %d is already connected: %d\n",i,i, tv[i]->clientstate );
            //continue;
        } else {
            LOG_INFO("%d: Do connect because client %d is not yet connected: %d\n",i,i, tv[i]->clientstate);
    //        LOG_INFO("BEFORE Connecting Client with clientId %s retry_connect %d state:%d\n"
     //       ,tv[i]->clientId, tv[i]->retry_connect, tv[i]->clientstate);
            /* connect tv->mqtt_client  */
            rc = do_connect(tv[i], MYCONNECT);
    
    #if 0
            if (rc != MQTTCLIENT_SUCCESS) {
                if (do_handle_mqtt_client_errors(tv[i],rc,__LINE__, __FUNCTION__,
                            "do_connect_array operation" )!= 0) {
                    CLIENT_LOG_ERROR(tv[i],"%d: Failed to connect MQTT Client with rc=%d\n",i,rc);
                }
            }
            LOG_INFO("AFTER Client with clientId %s retry_connect %d state:%d\n"
                ,tv[i]->clientId, tv[i]->retry_connect, tv[i]->clientstate);
    #endif
        }

        //LOG_INFO("end of loop i is %d\n", i);
        MY_ARRAY_CHECK_ERROR_LOOP_END
        //LOG_INFO("out of loop i is %d\n", i);
    }


    if (av.trueSleepAfterConnect>=0){
        LOG_STDOUT("Doing a true sleep( no mqtt client yield) for %d seconds after "
            " connect (make sure keepAlive is sufficient)\n", av.trueSleepAfterConnect );
        sleep(av.trueSleepAfterConnect);
        LOG_STDOUT("Done with true sleep \n");
    }


    return rc;
}

/*
 * Reconnect a synchronous MQTT client connection.
 *
 * This function will attempt to reconnect an MQTT client connection
 * for up to tv->retry_connect retries, before returning a failure. 
 * The caller should continue to to try again, as needed, 
 * until a failure is returned.
 *
 * @returns         non-zero in the event of a failure.
 *                 
 */
int do_reconnect(clientVariables *tv){
    int rc=0;
    while(tv->retry_connect>0) {
        if (tv->clientstate!=STATE_CONNECTING &&
            tv->clientstate!=STATE_FAILED_CONNECTION &&
            tv->clientstate!=STATE_DISCONNECTING  ){
            CLIENT_LOG_WARNING(tv,"Attempting to reconnect client\n");
            /* reconnect client  */
            rc = do_connect(tv, MYRECONNECT);
            if (rc != MQTTCLIENT_SUCCESS) {
                CLIENT_LOG_WARNING(tv,"Failed to reconnect MQTT Client with rc=%d, " \
                    "retry count=%d. Continue trying to reconnect...\n",
                    rc, tv->retry_connect);
                break; // changed back to add the break here on 4.11.13
                // check that variable.
            }else {
                CLIENT_LOG_INFO(tv,"Successfully reconnected client\n");
                break;
            }
        } /* else already trying to connect. don't bother it*/
    }

    if(tv->retry_connect<=0) {
        CLIENT_LOG_ERROR(tv,"Failed to reconnect MMQTT Client (Out of Retries)\n");
        tv->clientstate=STATE_FAILED_CONNECTION;
        rc=__LINE__; /* application should fail now. */
    }
    if (tv->mqtt_client == NULL){
        CLIENT_LOG_WARNING(tv,"Failed to reconnect MMQTT Client (tv->mqtt_client == NULL)\n");
        tv->clientstate=STATE_FAILED_CONNECTION;
        rc=__LINE__; /* application should fail now. */
    }
    return rc;
}

void do_disconnect_and_destroy_client(clientVariables *tv){
    tv->clientstate=STATE_DISCONNECTING;
    if (av.noDisconnect==0){
        /* Disconnect the tv->mqtt_client */
        if ( tv->primary_mqtt_client )  {
            wrap_MQTTClient_disconnect(tv->primary_mqtt_client, 0);
            CLIENT_LOG_INFO(tv,"Disconnected Primary client\n");
        }
        if ( tv->backup_mqtt_client )  {
            MQTTClient_disconnect(tv->backup_mqtt_client, 0);
            CLIENT_LOG_INFO(tv,"Disconnected HighAvailability client\n");
        }
    } else {
        CLIENT_LOG_INFO(tv,"NOT Disconnected(test parameter noDisconnect\n");
    }
    /* Free the memory used by the tv->mqtt_client instance */
    if ( tv->primary_mqtt_client )  {
        wrap_MQTTClient_destroy(&tv->primary_mqtt_client);
        CLIENT_LOG_INFO(tv,"Destroyed Primary client\n");
        tv->primary_mqtt_client=NULL;
    } 
    if ( tv->backup_mqtt_client )  {
        wrap_MQTTClient_destroy(&tv->backup_mqtt_client);
        CLIENT_LOG_INFO(tv,"Destroyed HighAvailability client\n");
        tv->backup_mqtt_client=NULL;
    }
}

void do_disconnect_and_destroy_array(clientVariables **tv){
    int i;
    for(i=0; i<tv[0]->total_clients; i++){
        LOG_STDOUT("CLEANUP - Disconnect and destroy client %d\n", i);
        do_disconnect_and_destroy_client(tv[i]);
    }

    do_status_update(tv[0],1); // force print out one last status

}
  
