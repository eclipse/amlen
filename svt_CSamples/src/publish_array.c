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
 * @file publish_array.c
 * 
 * The purpose of this file is to demonstrate publishing N messages using 
 * the MQTTv3 client library.
 * 
 * For more details one may refer to the MQTT client library in C 
 */




#include <mqttsample_array.h>

int do_publish_wait(clientVariables **tv, int i){
    int rc=0;
    /* Wait until the message has been delivered to the server */
    if ((av.qos>0) && (av.eightySixWaitForCompletion==0)){
        do { 
            CLIENT_LOG_INFO(tv[i],"Start WaitForCompletion for %llu: Previous rc = %d\n",
                    tv[i]->publish_count +1, rc);
            rc = wrap_MQTTClient_waitForCompletion(tv[i]->mqtt_client, tv[i]->mqtt_token, 
                WAIT_FOR_COMPLETION_TIMEOUT);
            if (rc!= MQTTCLIENT_SUCCESS) {
                if (do_handle_mqtt_client_errors(tv[i],rc,__LINE__, __FUNCTION__,
                    "MQTTClient_waitForCompletion operation" )!= 0) {
                    CLIENT_LOG_ERROR(tv[i],"Failed during wait for completion to deliver message, rc=%d\n",rc);
                    break;
                }
            } // else retry operation.
            if(tv[i]->connectionCounterLastPublish!=tv[i]->connectionCounter && // RTC 28794
                tv[i]->lastPublishURI!=tv[i]->curURI){                          // RTC 28794
                // RTC 28794 - we cannot trust the last wait for completion. Re-publish.    
                break;
            }
        } while ( rc != MQTTCLIENT_SUCCESS && !av.halt ) ; // fix defect RTC 18921

        if(tv[i]->connectionCounterLastPublish!=tv[i]->connectionCounter &&
            tv[i]->lastPublishURI!=tv[i]->curURI){
            // RTC 28794 - we cannot trust the last wait for completion. Re-publish.    
            CLIENT_LOG_WARNING(tv[i],"RTC 28794: Detected change to a new IMA server from old: %s . (new: %s) " \
                "cannot trust last publish due to HA switch to new server. Stop waitForCompletion attempt " \
                "immediately and republish last message. old connection counter : %d , (new: %d )\n",
                 tv[i]->lastPublishURI,tv[i]->curURI,tv[i]->connectionCounterLastPublish,tv[i]->connectionCounter  );
            rc=912; // caller should republish this message. 
        }

        CLIENT_LOG_INFO(tv[i],"End WaitForCompletion for %llu : rc = %d \n", 
            tv[i]->publish_count +1, rc);
    }

    return rc;

}

int do_publish_pass(clientVariables **tv){
    int rc=0;
    int orderModifier=0;
    struct shm_rw_region myshm;
    struct shm_rw_region myshm_w;
    unsigned long long shmvalblk=1000;
    unsigned long long lastshmval=0;
    unsigned long long sameshmval=0;
    unsigned long long curTime;
    unsigned long long lastTime;

    
    MY_ARRAY_INIT_VARIABLES;
    while (!av.halt ){

        if (av.info){
            do_info_display(av.tv, MYDETAILED,av.num_clients);
            //for(i=0; i<tv[0]->total_clients; i++){
                //do_info_display(tv[i]);
            //}
            av.info=0;
        }

        MY_ARRAY_CHECK_MESSAGES_PUBLISHED(av.msg_number, "Published All");
        MY_ARRAY_CHECK_ERROR_LOOP_START
        do_status_update(tv[0],0);

        if (av.sharedMemoryKey[0] != '\0' ){
            do_shm_read(&myshm,0);
            if (av.tv->rate_s.nmsgs > (myshm.count+av.sharedMemoryVal)){
                if(av.trace_level>1){
                    LOG_INFO("Synchronize pair: Publisher is too fast. Slowing down %llu ( av.tv->rate_s.nmsgs) > ((%llu + %llu) (myshm.count+av.sharedMemoryVal))\n", 
                      av.tv->rate_s.nmsgs,myshm.count , av.sharedMemoryVal);
                }
                sameshmval=0;
                shmvalblk=1;
                lastTime=getHRTimer();
                while(1){
                    curTime=getHRTimer();
                    if ((curTime-lastTime) >= MICROS_PER_SECOND ) {
                        if(av.trace_level>1){
                            LOG_INFO("Synchronize pair: doing a safe usleep\n");
                        }
                        do_safe_mqtt_client_usleep(tv[0],1); // Make sure we call the yield function and do status update every second at least
                        lastTime=curTime;
                    } else {
                        usleep(1);
                    }
                    lastshmval=myshm.count;
                    do_shm_read(&myshm,0);
                    if ((myshm.count) >= av.tv->rate_s.nmsgs ){
                        //if(av.trace_level>1){
                            LOG_STDOUT("Synchronize pair: Continuing publishing because we are in sync now: myshm.count: %llu  av.tv->rate_s.nmsgs : %llu\n", myshm.count, av.tv->rate_s.nmsgs );
                        //}
                        break;
                    }
                    if (myshm.count!=lastshmval ){
                        sameshmval=0;
                        continue;
                    }
                    if (myshm.count==lastshmval ){
                        sameshmval++;
                    }
                    if (sameshmval>=1000 && av.sharedMemoryCoupling==0 ){
                        if(av.trace_level>1){
                            LOG_STDOUT("Synchronize pair: Continuing publishing because myshm.count has not changed for av.sharedMemoryVal 1000 samples (~1 mSec)\n");
                        }
                        break;
                    }  
                    if (sameshmval==shmvalblk){
                        LOG_STDOUT("Synchronize pair: Publisher blocked for %llu samples, av.tv->rate_s.nmsgs:%llu , av.sharedMemoryCoupling=%d myshm.count=%llu\n", sameshmval, av.tv->rate_s.nmsgs, av.sharedMemoryCoupling, myshm.count);
                        shmvalblk=shmvalblk*10;
                    }

                }
                //do_safe_mqtt_client_sleep(tv[0],0);
            } else {
                if(av.trace_level>1){
                    LOG_STDOUT("Synchronize pair: Publisher is not too fast. myshm.count:%llu av.tv->rate_s.nmsgs: %llu av.sharedMemoryVal: %llu\n",
                                myshm.count,av.tv->rate_s.nmsgs,av.sharedMemoryVal);
                }
            }
        }

        do_throttle_check() ; // check if a throttle file exists to change the msg rate.
    
        //TODO - not supported to change msg with clients
        // TODO sprintf(mymesg, "%d:%s", tv->publish_count, g_msg); TODO
        //mqttMessage.payload = (char*)mymesg;
        //mqttMessage.payloadlen = strlen(mymesg);
        /* Publish the message */
       
        do_set_client_topic(tv[i], PUBLISH );
 
        if(av.orderMsg != 0 ){
            if (tv[i]->clientMsgBuf == NULL){
                tv[i]->clientMsgBuf=(char*)malloc(strlen(av.msg)+64);
            }
            if(av.sendOutOfOrderMsg != 0 ){
                if ((tv[i]->publish_count%3) == 0 && (tv[i]->publish_count > 1)){
                    orderModifier=-2; 
                } else {
                    orderModifier=0;
                } 
            }
            if(av.orderMsgStart > 0 ){
                orderModifier=orderModifier+av.orderMsgStart;
            }
            if (av.orderMsg == 4){  // orderMsg==4 setting, print order info at the end of the message -use with mdbmessages.jar
                sprintf(tv[i]->clientMsgBuf, "%s|ORDER:%s:%llu:%llu:%d",av.msg, tv[i]->clientId, 
                tv[i]->publish_count+1+orderModifier, 
                getHRTimer(),
                i);
                av.tv->orderMsgLastNumber=tv[i]->publish_count+1+orderModifier;
            } else if ( av.topicMultiTest!=0 ){
                sprintf(tv[i]->clientMsgBuf, "ORDER:%s:%llu:%llu:%d:%s", tv[i]->clientId, 
                (tv[i]->publish_count/av.topicMultiTest)+1+orderModifier, 
                getHRTimer(),
                i, // TODO - remove? 11.8.13 - can be used to correlate many (pubs) to 1 on the subscriber side. - not in use yet... do we need this?
                av.msg);
                // av.tv->orderMsgLastNumber can be used to detect where the "gap" is. only for 1 to 1 publish/subscribe pairs.
                av.tv->orderMsgLastNumber=tv[i]->publish_count+1+orderModifier;
            } else {
                sprintf(tv[i]->clientMsgBuf, "ORDER:%s:%llu:%llu:%d:%s", tv[i]->clientId, 
                tv[i]->publish_count+1+orderModifier, 
                getHRTimer(),
                i, // TODO - remove? 11.8.13 - can be used to correlate many (pubs) to 1 on the subscriber side. - not in use yet... do we need this?
                av.msg);
                // av.tv->orderMsgLastNumber can be used to detect where the "gap" is. only for 1 to 1 publish/subscribe pairs.
                av.tv->orderMsgLastNumber=tv[i]->publish_count+1+orderModifier;
            }
            tv[i]->mqttMessage.payload = (char*)tv[i]->clientMsgBuf;
            /* the length of the payload does not need to include the nul char*/
            tv[i]->mqttMessage.payloadlen = strlen(tv[i]->clientMsgBuf);
            tv[i]->mqttMessage.qos = av.qos;
            tv[i]->mqttMessage.retained = av.retained_flag;
        } else {
            tv[i]->mqttMessage.payload = (char*)av.msg;
            /* the length of the payload does not need to include the nul char*/
            tv[i]->mqttMessage.payloadlen = strlen(av.msg);
            tv[i]->mqttMessage.qos = av.qos;
            tv[i]->mqttMessage.retained = av.retained_flag;
        }

        if ( tv[i]->mqttMessage.payloadlen > 80 && av.trace_level<2 ){ 
        CLIENT_LOG_INFO(tv[i],"%d: %llu:Publishing to topic \"%s\" qos %d, message \"%.*s ... (%d chars truncated)\" \n", 
           i, tv[i]->publish_count +1, tv[i]->topic, av.qos, 80, (char*) tv[i]->mqttMessage.payload , tv[i]->mqttMessage.payloadlen - 80 );
        } else {
        CLIENT_LOG_INFO(tv[i],"%d: %llu:Publishing to topic \"%s\" qos %d, message \"%s\" \n", 
           i, tv[i]->publish_count +1, tv[i]->topic, av.qos, (char*) tv[i]->mqttMessage.payload  );
        }
    
        rc = wrap_MQTTClient_publishMessage(tv[i]->mqtt_client, (char*)tv[i]->topic, 
            &tv[i]->mqttMessage, &tv[i]->mqtt_token);
    
        if (rc == MQTTCLIENT_SUCCESS ) {
            tv[i]->lastActionURI=tv[i]->curURI;                             // RTC 25776 - used for failover
            tv[i]->lastPublishURI=tv[i]->curURI;                            // RTC 28794
            tv[i]->connectionCounterLastPublish=tv[i]->connectionCounter;   // RTC 28794

            if (av.sharedMemoryKey[0] != '\0' ){
                myshm_w.count=av.tv->rate_s.nmsgs;
                myshm_w.rate=0;
                do_shm_write(&myshm_w,1);
            }

            if(av.qos>0){
                if (av.eightySixWaitForCompletion!=0){
                    CLIENT_LOG_INFO(tv[i],"Overide flag set: %d - no wait for completion. \n", av.eightySixWaitForCompletion);
                } else if (av.waitForCompletionMode==0) {
                    CLIENT_LOG_INFO(tv[i],"Process wait for completion in single wait mode - wait after each publish\n");
                    /* Single wait mode - Wait until the message has been delivered to the server after each publish */
                    /* Should be slower but higher reliability especially for events like failover.*/
                    rc=do_publish_wait(tv, i);
                    if (rc!=0){
                        CLIENT_LOG_INFO(tv[i],"Failed wait for completion republish last message: rc : %d\n",rc);
                        continue; // republish the last message.
                    }
                } else {
                    CLIENT_LOG_INFO(tv[i],"Process wait for completion in bulk mode - wait wait will be done later in batch mode \n");
                }
            }

            if (rc == MQTTCLIENT_SUCCESS ) {
                tv[i]->publish_count++;
                tv[i]->clientstate=STATE_PUBLISHING;
                do_rate_control(tv[i],i);
            } else {
                if (do_handle_mqtt_client_errors(tv[i],rc,__LINE__, __FUNCTION__,
                        "MQTTClient_publish or wfc operation" )!= 0) {
                    CLIENT_LOG_ERROR(tv[i],"Failed to deliver message, rc=%d\n",rc);
                    break;
                } // else retry operation.
            }
        } else {
            if (do_handle_mqtt_client_errors(tv[i],rc,__LINE__, __FUNCTION__,
                     "Message not accepted for delivery" )!= 0) {
                CLIENT_LOG_ERROR(tv[i],"Failed to deliver message, rc=%d\n",rc);
                //break;
                continue;
            }
        }
        if (av.usleepLoop>=0) {
            do_safe_mqtt_client_usleep(tv[0],av.usleepLoop);
        }
        MY_ARRAY_CHECK_ERROR_LOOP_END

        //-----------------------------------------
        // Wait for completion bulk processing mode - should be faster to do in batches.
        //-----------------------------------------
        if (av.waitForCompletionMode==1 && av.qos>0 && av.eightySixWaitForCompletion==0) {
            CLIENT_LOG_INFO(tv[0],"Starting. Process wait for completion in bulk mode / batch mode \n");
            MY_ARRAY_CHECK_ERROR_LOOP_START
            /* Wait until the message has been delivered to the server, for all clients at once. */
            rc=do_publish_wait(tv, i);
            if (rc!=0){
                LOG_ERROR("Program cannot toleraate a wait for completion error in bulk processing mode.\n");
                HALT_ERROR ("Errors not tolerated in this mode. Additional code would be needed.\n");
                continue; // republish the last message.
            }
            MY_ARRAY_CHECK_ERROR_LOOP_END
            CLIENT_LOG_INFO(tv[0],"Done. Processed wait for completion in bulk mode / batch mode \n");
        }

        if (av.sleepLoop>=0) {
            do_safe_mqtt_client_sleep(tv[0],av.sleepLoop);
        }

        return 0;
    }
    return 1; 
}

/*
 * Publish MQTTv3 messages
 *
 * @returns         MQTT_CLIENT_SUCCESS ( 0 ) if everything succeeded or
 *                  non-zero in the event of a failure.
 */
void* do_publish(void *v){
    clientVariables **tv=(clientVariables**)v;
    int rc=0,i=0;

    // connect client  
    rc = do_connect_array(tv);
    if (rc != 0){
        LOG_ERROR("Failed to connect array of clients");
        return NULL;
    }

    if (av.appstate==STATE_WAIT_TO_PUBLISH ) {
        LOG_STDOUT("In appstate av.appstate==STATE_WAIT_TO_PUBLISH : %d. Waiting for signal.\n", av.appstate);
        while(av.appstate==STATE_WAIT_TO_PUBLISH && !av.halt){
            do_safe_mqtt_client_usleep(tv[0],100000);
            //#usleep(100000); 
        }
        LOG_STDOUT("Signal received - av.appstate== : %d. \n", av.appstate);
    }

    LOG_STDOUT("Publishing - Ready\n");

    while (!av.halt ){
        if (do_publish_pass(tv) != 0) {break;}
    }

    for(i=0; i<tv[0]->total_clients; i++){
        LOG_STDOUT("%d: Published %llu messages to topic %s . Note: if special topic " \
        "change test flags were used, it would not be indicated here.\n",
        i, tv[i]->publish_count, av.topic);
    }

    do_disconnect_and_destroy_array(tv);

    return NULL; 
}
