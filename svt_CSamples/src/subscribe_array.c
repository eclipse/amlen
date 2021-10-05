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
 * @file subscribe_array.c
 * 
 * The purpose of this file is to demonstrate subscribing to N messages using 
 * the MQTTv3 client library.
 * 
 * For more details one may refer to the MQTT client library in C 
 */

#include <mqttsample_array.h>


int do_single_subsription( clientVariables *tv ){
    int rc;

    do_set_client_topic(tv, SUBSCRIBE);

    CLIENT_LOG_STDOUT(tv,"Subscribing to topic \"%s\" qos %d for client\n", tv->topic, av.qos);

    /* Subscribe to the tv->topic */
    rc = wrap_MQTTClient_subscribe(tv->mqtt_client,(char*) tv->topic, av.qos);
    if (rc != MQTTCLIENT_SUCCESS) {
        tv->lastActionURI=tv->curURI;                             // RTC 25776 - used for speedy failover
        if (do_handle_mqtt_client_errors(tv,rc,__LINE__, __FUNCTION__,
                    "MQTTClient_subscribe operation" )!= 0) {
            CLIENT_LOG_ERROR(tv,"Failed to subscribe, rc=%d\n", rc);
            //av.halt=1; // discontinue further processing TODO------- This isnt right. client shouldn't kill all others
            //goto out;
        } // else expect caller to retry the operation.
    } else {
        tv->subscribeTime=getHRTimer();
        CLIENT_LOG_STDOUT(tv,"Subscribed - Ready\n");
        //break;
    }
    return rc;
}

int do_subscribe_pass( clientVariables **tv){
    int rc;
    MY_ARRAY_INIT_VARIABLES;

    while(!av.halt){
        MY_ARRAY_CHECK_CLIENT_STATE_EQ(STATE_SUBSCRIBED,"Subscribed");
        MY_ARRAY_CHECK_ERROR_LOOP_START
        do_status_update(tv[0],0);
        if (tv[i]->clientstate==STATE_SUBSCRIBED){
            continue;
        } 
        rc=do_single_subsription(tv[i]);
        if (rc == MQTTCLIENT_SUCCESS) {
            tv[i]->clientstate=STATE_SUBSCRIBED;
        }
        MY_ARRAY_CHECK_ERROR_LOOP_END
        return 0;
    }
    return 1;

}

int do_receive_pass( clientVariables **tv){
    int rc;
    long long order;
    char *tok=NULL;
    unsigned long long latency_start;
    unsigned long long receive_start;
    unsigned long long latency_end;
    unsigned long long curTime;
    char *saveptr=NULL;
    MY_ARRAY_INIT_VARIABLES;
    MQTTClient_message* message = NULL;
    char* receivedTopicName = NULL;
    long receiveTimeout = 100l;
    long actual_receiveTimeout = 100l;
    int topicLen;
    struct shm_rw_region myshm_r;
    struct shm_rw_region myshm;
    myshm.rate=0;

    if (av.userReceiveTimeout >= 0) {
        receiveTimeout=av.userReceiveTimeout;
        //CLIENT_LOG_INFO(tv[i],"Using User specified receiveTimeout %lu\n", receiveTimeout=av.userReceiveTimeout);
    }

    while (!av.halt ){
        if (av.info){
            do_info_display(av.tv,MYDETAILED,av.num_clients);
            //for(i=0; i<tv[0]->total_clients; i++){
                //do_info_display(tv[i]);
            //}
            av.info=0;
        }
        MY_ARRAY_CHECK_MESSAGES_RECEIVED(av.msg_number, "Received All");
        MY_ARRAY_CHECK_ERROR_LOOP_START


        if ((av.checkConnection>0) && 
            (av.checkConnectionCounter > av.checkConnection) &&
            (av.tv->rate_s.nmsgs > 0) 
             ){
            av.checkConnectionCounter=0; // only do this every time a lot timeouts or failures in a row have occurred.
            CLIENT_LOG_STDOUT(tv[i],"Start - Checking connections, there seems to be some problem with clients being connected\n");
            rc = do_check_array_connections(av.tv_array, tv[i]->curURI);
            CLIENT_LOG_STDOUT(tv[i],"Complete - rc: %d - Done Checking connections\n",rc);
        }

        if (av.dynamicReceiveTimeout > 0) {
            if (av.tv->rate_s.nmsgs > 0) { 
                if(av.trace_level>1){
                    CLIENT_LOG_INFO(tv[i],"Dynamically adjusting recieveTimeout %ld \n", receiveTimeout);
                }
                actual_receiveTimeout=receiveTimeout+(tv[i]->receiveTimeoutExpired*100);
                if(av.trace_level>1){
                    CLIENT_LOG_INFO(tv[i],"After adjustment recieveTimeout %ld\n", actual_receiveTimeout);
                }
            } else {
                actual_receiveTimeout=receiveTimeout;
            }
        } else {
            actual_receiveTimeout=receiveTimeout;
        }
        
        do_status_update(tv[0],0);

        do_throttle_check () ; // check if a throttle file exists to change the msg rate.
    
        receivedTopicName = NULL;
        topicLen=0;
        if(av.trace_level>1){
            CLIENT_LOG_INFO(tv[i], "Attempt to receive w/ actual_receiveTimeout %ld \n", actual_receiveTimeout);
        }
        receive_start=getHRTimer();
        /* Check to see if a message is available */
        rc = wrap_MQTTClient_receive(tv[i]->mqtt_client, &receivedTopicName, &topicLen, 
                          &message, actual_receiveTimeout);
        if (rc == MQTTCLIENT_SUCCESS && message != NULL) {
            latency_end=getHRTimer();
            CLIENT_LOG_INFO(tv[i], "receive time needed: %llu\n", latency_end-receive_start);
            av.checkConnectionCounter=0;
            tv[i]->lastActionURI=tv[i]->curURI;                             // RTC 25776 - used for speedy failover
            /* A message has been received */

            if ((av.retainedMsgCounted == 1 && message->retained == 1) ||  (message->retained == 0)){
                tv[i]->clientstate=STATE_RECEIVING;
                tv[i]->subscribe_count++;
                do_rate_control(tv[i],i);
                if (av.sharedMemoryKey[0] != '\0' && av.sharedMemoryType==0 ){
                    myshm.count=av.tv->rate_s.nmsgs;
                    do_shm_write(&myshm, 0);
                    if(av.trace_level>1){
                        LOG_INFO("(A) Logging do_shm_write: %llu\n", myshm.count);
                    }
                }
            
            }

            if ((av.retainedMsgCounted == 1 && message->retained == 1) ||  (message->retained == 0)){
              if ( message->payloadlen > 80 && av.trace_level<2 ){
                CLIENT_LOG_INFO(tv[i], "Message %llu " \
                "received on topic '%s': " \
                "r: %d " \
                "d: %d " \
                "i: %d " \
                "m: %.*s(%d characters not displayed)\n",
                tv[i]->subscribe_count,
                receivedTopicName, 
                message->retained, 
                message->dup, 
                message->msgid, 
                80, (char*)message->payload, message->payloadlen-80);
              } else {
                CLIENT_LOG_INFO(tv[i], "Message %llu " \
                "received on topic '%s': " \
                "r: %d " \
                "d: %d " \
                "i: %d " \
                "m: %.*s\n", 
                tv[i]->subscribe_count,
                receivedTopicName, 
                message->retained, 
                message->dup, 
                message->msgid, 
                message->payloadlen, (char*)message->payload);
              }
            } else if ( message->retained == 1) {
              if ( message->payloadlen > 80 && av.trace_level<2 ){
                CLIENT_LOG_INFO(tv[i],"Message xx " \
                "received on topic '%s': " \
                "r: %d(retained message not counted) " \
                "d: %d " \
                "i: %d " \
                "m: %.*s(%d characters not displayed)\n",
                receivedTopicName, 
                message->retained, 
                message->dup, 
                message->msgid, 
                80, (char*)message->payload, message->payloadlen-80);
              } else {
                CLIENT_LOG_INFO(tv[i], "Message xx " \
                "received on topic '%s': " \
                "r: %d(retained message not counted) " \
                "d: %d " \
                "i: %d " \
                "m: %.*s\n", 
                receivedTopicName, 
                message->retained, 
                message->dup, 
                message->msgid, 
                message->payloadlen, (char*)message->payload);
              }
            } else {
                HALT_ERROR("Internal bug. should not reach this line.\n");
            }

            if ((av.retainedMsgCounted == 1 && message->retained == 1) ||  (message->retained == 0)){

                if (av.verifyMsg && !av.msg_buf) { //
                    HALT_ERROR("USER error specified av.verifyMsg , but did not specify -m message to verify\n");
                }
    
                if (av.orderMsg){
                    tok=strtok_r(message->payload,":",&saveptr);
                    tok=strtok_r(NULL,":",&saveptr);
                    tok=strtok_r(NULL,":",&saveptr);
                    order=strtoull(tok,NULL,10); 
                    tok=strtok_r(NULL,":",&saveptr);
                    latency_start=strtoull(tok,NULL,10); 
                    tok=strtok_r(NULL,":",&saveptr);
                
                    //CLIENT_LOG_STDOUT(tv[i], "1now saveptr is %s\n", saveptr);
                    //tok=strtok_r(NULL,":",&saveptr);

                    //------------------------------------------
                    // Base shm write off of recieved order not internal message counters
                    //------------------------------------------
                    if (av.sharedMemoryKey[0] != '\0' && av.sharedMemoryType==1 ){
                        myshm.count=order;
                        do_shm_write(&myshm, 0);
                        if(av.trace_level>1){
                            LOG_INFO("(B) Logging do_shm_write: %llu\n", myshm.count);
                        }
                    }

                    //------------------------------------------
                    // order gap detection
                    //------------------------------------------
                    if ((av.orderMsg==3) && ( order != (tv[i]->orderMsgLastNumber+1))){
                        LOG_STDOUT("gap detected between %llu and %llu \n" , order, tv[i]->orderMsgLastNumber);
                        if (av.sharedMemoryKey[0] != '\0' ){
                            do_shm_read(&myshm_r, 1);
                            LOG_STDOUT("publisher count:%llu rate:%llu \n" , myshm_r.count, myshm_r.rate);
                        }
                    }

                    //------------------------------------------
                    // msg ordering checks
                    //------------------------------------------
                    if(tv[i]->orderMsgLastNumber==0 && tv[i]->subscribe_count <= 1){
                        tv[i]->orderMsgLastNumber=order;
                    } else if (av.qos == 0 && (order > (tv[i]->orderMsgLastNumber))){
                        tv[i]->orderMsgLastNumber=order;
                    } else if (av.qos == 1 && ((order == (tv[i]->orderMsgLastNumber)) || (order == tv[i]->orderMsgLastNumber+1))){
                        tv[i]->orderMsgLastNumber=order;
                    } else if (av.qos == 2 && (order == (tv[i]->orderMsgLastNumber+1))){
                        tv[i]->orderMsgLastNumber=order;
                    } else {
                        if (av.orderMsg==1){
                            CLIENT_LOG_WARNING(tv[i],"%d: (single client - not implemented for multiple clients)Verify" \
                            " order failed, order:%llu tv[i]->orderMsgLastNumber:%llu, qos:%d\n",
                                i,order,tv[i]->orderMsgLastNumber, av.qos );
                            av.testCriteriaOrderMsg++;
                        }
                        tv[i]->orderMsgLastNumber=order;
                    }

    
                    //------------------------------------------
                    // order range checks
                    //------------------------------------------
                    if (av.testCriteriaOrderMin_enabled==1 && order < av.testCriteriaOrderMinVal ){
                        CLIENT_LOG_WARNING(tv[i],"(single client - not implemented for multiple clients)Verify" \
                        " order failed, Violated min range: %llu order:%llu tv[i]->orderMsgLastNumber:%llu, qos:%d\n",
                            av.testCriteriaOrderMin,order,tv[i]->orderMsgLastNumber, av.qos );
                        av.testCriteriaOrderMin++;
                    }else if (av.testCriteriaOrderMax_enabled==1 && (order > av.testCriteriaOrderMaxVal) ){
                        CLIENT_LOG_WARNING(tv[i],"(single client - not implemented for multiple clients)Verify" \
                        " order failed, Violated max range: %llu order:%llu tv[i]->orderMsgLastNumber:%llu, qos:%d\n",
                            av.testCriteriaOrderMax,order,tv[i]->orderMsgLastNumber, av.qos );
                        av.testCriteriaOrderMax++;
                    }

                    CLIENT_LOG_INFO(tv[i],"latency %f\n",(latency_end-latency_start)/MICROS_PER_SECOND);
                    //CLIENT_LOG_STDOUT(tv[i],"pub_array_index=%d\n", pub_array_index);
                    
                    if (av.verifyMsg) {  // verify message contents
                        if (memcmp(saveptr, av.msg_buf, av.msg_buf_len) != 0 ){
                            CLIENT_LOG_WARNING(tv[i],"Failed to verify message contents matched input -m buffer (B)\n");
                            CLIENT_LOG_WARNING(tv[i],"Verification failure: Expected %.*s\n",  av.msg_buf_len, (char*)av.msg_buf );
                            CLIENT_LOG_WARNING(tv[i],"Verification failure: Actual   %.*s\n",  av.msg_buf_len, (char*)saveptr    );
                            av.testCriteriaVerifyMsg++;
                        }
                    }
                } else if (av.verifyMsg) {  // verify message contents
                    if (memcmp(message->payload, av.msg_buf, av.msg_buf_len)  != 0 ){
                        CLIENT_LOG_WARNING(tv[i],"Failed to verify message contents matched input -m buffer (A)\n");
                        CLIENT_LOG_WARNING(tv[i],"Verification failure: Expected %.*s\n",  av.msg_buf_len,(char*) av.msg_buf );
                        CLIENT_LOG_WARNING(tv[i],"Verification failure: Actual   %.*s\n",  av.msg_buf_len,(char*) message->payload);
                        av.testCriteriaVerifyMsg++;
                    }
                }
            }
    
            wrap_MQTTClient_freeMessage(&message);
            wrap_MQTTClient_free(receivedTopicName);
    
            MQTTCLIENT_ERROR_INJECT_DISCONNECT(tv[i]->mqtt_client);
        } else if (rc == MQTTCLIENT_SUCCESS &&  message == NULL) {
            av.checkConnectionCounter++;
            tv[i]->receiveTimeoutExpired++;
            if(av.trace_level>1){
                CLIENT_LOG_INFO(tv[i], "receive timeout expired: %llu\n", tv[i]->receiveTimeoutExpired);
            }
        } else if (rc != MQTTCLIENT_SUCCESS) {
            if (do_handle_mqtt_client_errors(tv[i],rc,__LINE__, __FUNCTION__,
                        "MQTTClient_receive operation" )!= 0) {
                CLIENT_LOG_ERROR(tv[i],"%d: Failed to receive message, rc=%d\n",i,rc);
                //break; 
                continue;
            }
        }
        
        if (av.msgTimeoutAfterSub >0 && tv[i]->subscribe_count==0 ){
            curTime=getHRTimer();
            if (curTime>(tv[i]->subscribeTime+av.msgTimeoutAfterSub )){
                CLIENT_LOG_STDOUT(tv[i],"TIMEOUT! av.msgTimeoutAfterSub expired \n");
                return 1;  // stop receiving messages.
            }
        }
        if (av.unsubscribeAfterRecv == 1){
            rc=wrap_MQTTClient_unsubscribe(tv[i]->mqtt_client,tv[i]->topic);
            CLIENT_LOG_STDOUT(tv[i],"Unsubscribe rc:%d.\n",rc);
        }
        if (av.subscribeAfterRecv == 1){
            rc=do_single_subsription( tv[i] );
            CLIENT_LOG_STDOUT(tv[i],"do_single_subsription rc:%d.\n",rc);
        }
        MY_ARRAY_CHECK_ERROR_LOOP_END
        if (av.sleepLoop>=0) {
            do_safe_mqtt_client_sleep(tv[0],av.sleepLoop);
        }

        return 0;
    }
    return 1;
}

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

void* do_subscribe(void *v){
    clientVariables **tv=(clientVariables**)v;
    int rc=0, i=0;

    // connect client  
    rc = do_connect_array(tv);
    if (rc != 0){
        LOG_ERROR("Failed to connect array of clients with rc %d",rc);
        return NULL;
    }

    if (av.skipSubscribe != 1 ){
        LOG_INFO("Subscribing all clients\n");
        while (!av.halt ){
            if (do_subscribe_pass(tv) != 0) {break;}
        }
    } else {
        LOG_STDOUT("OVERRIDE: USER requested to skip the subscribe. av.skipSubscribe=%d\n", 
            av.skipSubscribe);
    }

    if (av.appstate==STATE_WAIT_TO_RECEIVE ) {
        LOG_STDOUT("In appstate av.appstate==STATE_WAIT_TO_RECEIVE : %d. Waiting for signal.\n", av.appstate);
        while(av.appstate==STATE_WAIT_TO_RECEIVE && !av.halt){
            do_safe_mqtt_client_usleep(tv[0],100000);
            //#usleep(100000); 
        }
        LOG_STDOUT("Signal received - av.appstate== : %d. \n", av.appstate);
    }

    //LOG_INFO("sleep"); sleep (36000);
    

    LOG_INFO("Begin receiving messages\n");
    while (!av.halt ){
        if (do_receive_pass(tv) != 0) {break;}
    }

    for(i=0; i<tv[0]->total_clients; i++){
        LOG_STDOUT("%d: Received %llu messages.\n",i, tv[i]->subscribe_count);
    }

    if (av.waitAfterReceive==1) {
        av.appstate=STATE_WAIT_AFTER_RECEIVE;
        LOG_STDOUT("In appstate av.appstate == av.appstate=STATE_WAIT_AFTER_RECEIVE : %d. Waiting for signal.\n", av.appstate);
        while(av.appstate==STATE_WAIT_AFTER_RECEIVE && !av.halt){
            do_safe_mqtt_client_usleep(tv[0],100000);
            //#usleep(100000); 
        }
        LOG_STDOUT("Signal received - av.appstate== : %d. \n", av.appstate);
    }

    if (av.unsubscribe) {
        for(i=0; i<tv[0]->total_clients; i++){
            rc=wrap_MQTTClient_unsubscribe(tv[i]->mqtt_client,tv[i]->topic);
            LOG_STDOUT("%d: Unsubscribe rc:%d.\n",i,rc);
        }
    } 

    do_disconnect_and_destroy_array(tv);

    return NULL;

}
