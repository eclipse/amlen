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


#include <mqttsample_array.h>

/*
 * @file mqttsample_array.c
 *
 * MQTT C Sample that uses MQ's client library for C to publish or subscribe
 * to a topic. It demonstrates a synchronous usage of the client library.
 * 
 * Command line options:
 * 
 * -s serverURI : The URI address of the ISM server. This is in the format of
 *  tcp://<ip address>:<port> This is a required parameter.
 * 
 * -a action    : Either the String: publish or subscribe. This is a required 
 *  parameter.
 * 
 * -c true|false: Set the cleansession flag to true or false
 * 
 * -t topicName : The name of the topic on which the messages are published. 
 * The default topic name is /MQTTV3Sample/C
 * 
 * -i string    : Set the clientId to string.
 * 
 * -m message   : A String representing the message to be sent. 
 * The default message is: "Message from MQTTv3 C client"
 * 
 * -n count     : The number of times the specified message is to be sent.
 *  The default number of message sent or received is 1.
 * 
 * -q qos       : The Quality of Service level 0, 1, or 2 The default QOS is 0.
 * 
 * -o filename  : Set the logfile. The log defaults to stdout and stderr .
 * 
 * -p password  : Set the password
 * 
 * -r directory : Enable persistence and specify datastore directory. 
 *                The default persistence is false.
 * 
 * -u user      : Set the user 
 * 
 * -t topic     : Set the topic
 * 
 * -v           : indicates verbose output
 * 
 * -w number    : rate to send messages
 * 
 * -z number    : number of array clients
 * 
 * -h output usage statement
 * 
 */


/*
 * Initialize global variables
 */
appVariables av={ 0,0,NULL,NULL,1,0,"","","","","","","","Message from MQTTv3 C client",
                  NULL, NULL, NULL,"",MQTTCLIENT_PERSISTENCE_NONE,0,0,1,0,0,0,1,NULL,0,
                  NULL,NULL,NULL,NULL,NULL,1,0,0,0,0,0, 60*MICROS_PER_SECOND  ,0,0,0,0,
                  0,0,0,0,0,0,0,-1,-1,0,10,0,0,-1,NULL,0,-1,-1,0,0,0,0,0,0,0,0,-1,
                  0,0,99999999,0,0,0,0,0,0,1,0,0,NULL,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0.0,"",  // last col is: userLtpaFile
                  "",0,999999999,0,1000000,0,0,"",0,0.0,0,0,0,"",0,NULL,0,"" ,1,0,         // last col is: sharedMemoryInit
                  0,0,"",0,0,0,0,0,0,0,0,0,-1,0,"",0,0,0,0,100,0,0,0,0,-1,0,NULL,NULL,0,   // last col is: topic_buf_plen
                  0,0,0,0, NULL, "a will message", 0, 0};
                  //0,0,99999999,0,0,0,0,0,0,1,0,0,NULL,0,0,0,0,0,0,4,5,6,7,8};

/*
MQTTClient g_mqtt_client;
MQTTClient_deliveryToken g_mqtt_token;
unsigned int g_msg_number=1;    
unsigned int g_publish_count=0; 
unsigned int g_subscribe_count=0;
unsigned int g_warning_count=0;
unsigned int g_error_count=0;
unsigned int g_trace_level=0;   
FILE *g_log2out=NULL;          
FILE *g_log2err=NULL;         
int g_retry_connect=10; // arbitrary setting. 
rateControl g_rate_s={0,0,0,0,0,0,0,0.0,0.0,0.0};
rateControl *g_rate=NULL;
*/

void check_client_rc(int rc,int line ,const char*file){
    if(rc!=0){
        HALT_ERROR("pclient call returned non-zero: rc : %d",rc);
    }
    LOG_INFO("rc %d for %d\n", rc, line);
}


int start_clients(void){
    int rc=0;
    int i=0;
    time_t t_result; 
    char host[MAX_BUF_SZ]="";
    char buffer[MAX_BUF_SZ]="";
//    pthread_t *mythreads[av.num_clients];
    clientVariables **mytv=malloc(av.num_clients* sizeof(clientVariables*));
    clientVariables *tv;
    int array_id=0;
    gethostname(host,MAX_BUF_SZ);
    time(&t_result);
    srand((unsigned int)t_result+getpid());
    av.tv_array=mytv;

    /* create and run all clients */ 
    for(i=0; i<av.num_clients; i++){
        array_id=i;
 //       mythreads[i]=(pthread_t *)malloc(sizeof(pthread_t));
        mytv[i]=(clientVariables *)malloc(sizeof(clientVariables));
        tv=mytv[i];
        memset(tv, 0, sizeof(clientVariables));
        memset(tv->topicCNPrepend, 0, sizeof(tv->topicCNPrepend));
        memset(tv->topicUSPrepend, 0, sizeof(tv->topicUSPrepend));
        tv->array_id=array_id;
        tv->topicCNPrepend[0]='\0';
        tv->topicUSPrepend[0]='\0';
        strncpy(tv->mqttMessage.struct_id,"MQTM",4);
        tv->retry_connect=10;
        tv->clientstate=STATE_INITIALIZING;
        tv->total_clients=av.num_clients;
        tv->retry_connect=av.retry_connect;
        if (av.desiredMsgsPerSec!=0){
            tv->rate=&tv->rate_s;
            tv->rate->desiredMsgsPerSec=av.desiredMsgsPerSec;
        }
        //TODO intialize the tv

        if (av.useCNasClientID!=0){
            // note below normally av.useWrongCNasClientID == 0 unless user doing error path testing.
            sprintf(buffer, "CN%07d%s", av.dccIncrementer+tv->array_id+av.useWrongCNasClientID,av.clientId);
        }else if (av.scaleTest!=0){
            // For "scaleTest" clientId must match an expected general format:
            //  pClientId = prefix + host + "_" + pid + "_" + tid;
            sprintf(buffer, "%.1s%s_%d_%d%08x%s",av.action,host,getpid(),array_id,rand(),av.clientId);
        }else if (av.badTopicChangeTest!=0){ 
            // I think this can be used to cause the "CONNECT must the first message in a connection" error
            if (av.clientId[0]!= '\0' ){
                sprintf(buffer, "%s",av.clientId);
            }
        }else{
            if (av.clientId[0]!= '\0' && av.num_clients > 1){
                sprintf(buffer, "%d_%s",i, av.clientId);
            } else if (av.clientId[0]!= '\0' && av.num_clients == 1){
                sprintf(buffer, "%s",av.clientId);
            } else {
                sprintf(buffer, "%.1s%d%d%08x%s%s",av.action,array_id,getpid(),rand(),host,av.clientId);
            }
        }
        /*
        * Per MQTT Spec, clientId must not be longer than 23 chars. 
        * Enforce the 23 char limit.
        */
        sprintf(tv->clientId, "%.23s", buffer);
        do_info_display_setup(tv);
        //check_thread_rc(rc, __LINE__, __FILE__);
    }

    if(strcmp(av.action, PUBLISH_WAIT) == 0){
            //LOG_INFO("started thread %d, clientId:%s\n",threadId,tv->clientId);
    //        rc=pthread_create(mythreads[i], NULL, do_publish,(void*)tv);
            LOG_INFO("starting %d publishers that wait for user signal 10 " \
                    " before starting to publish\n", av.num_clients);
            av.appstate=STATE_WAIT_TO_PUBLISH;
            do_publish(mytv);
    } else if(strcmp(av.action, PUBLISH) == 0){
            LOG_INFO("starting %d publishers\n", av.num_clients );
            do_publish(mytv);
    } else if(strcmp(av.action, RECEIVE_WAIT) == 0){
            LOG_INFO("starting %d subscribers that wait for user signal 10 " \
                    " before starting to receive\n", av.num_clients);
            av.appstate=STATE_WAIT_TO_RECEIVE;
            do_subscribe(mytv);
    } else if (strcmp(av.action, SUBSCRIBE) == 0){
            LOG_INFO("starting %d subscribers\n", av.num_clients);
            do_subscribe(mytv);
    } else {
           HALT_ERROR("Invalid action parameter: %s\n", av.action);
   }

    do_info_display(av.tv,MYDETAILED,av.num_clients);
    if(av.trace_level>1){
        for(i=0; i<av.num_clients; i++){
            LOG_INFO("Displaying client %d\n",i);
            if (av.trace_level>0){
                do_info_display(mytv[i],MYDETAILED,1);
            }
            free(mytv[i]);
        }
    }
    free(mytv);
    
    return rc;
}

int main(int argc, char **argv){
    int rc=0;
    int svt_test_result=0;
    int svt_test_criteria=0;
    av.log2out=stdout; 
    av.log2err=stderr;
    signal(SIGUSR1, do_signal_handle);
    signal(SIGUSR2, do_signal_handle);
    signal(SIGINT, do_signal_handle);
    signal(SIGTERM, do_signal_handle);

    av.lastStatTime=getHRTimer();
    
    /* Process user inputs */
    do_parse_arguments(argc, argv);

    av.tv=(clientVariables *)malloc(sizeof(clientVariables));
    memset(av.tv, 0, sizeof(clientVariables));
    av.tv->rate=&av.tv->rate_s;
    av.tv->rate->desiredMsgsPerSec=av.desiredMsgsPerSec;
    do_info_display_setup(av.tv);

    rc=start_clients();

    LOG_STDOUT("SVT_TEST_COMPLETE %s rc=%d\n", av.action, rc);

    errno=0;
    
    if (av.testCriteriaPercentMessage_enabled) {
        // Criteria must be incremented for each enabled criteria
        svt_test_criteria++;
        if((double)av.testCriteriaPercentMessage>   
            ((((double)av.tv->rate_s.nmsgs)/((double)av.msg_number*av.num_clients))*100.0) ) { 
            LOG_ERROR("SVT_TEST_CRITERIA: Failed av.testCriteriaPercentMessage %% messages (expected) %f > %f (actual)\n",
                (double)av.testCriteriaPercentMessage , 
                ((((double)av.tv->rate_s.nmsgs)/((double)av.msg_number*av.num_clients))*100.0) );
            svt_test_result++;
        }else {
            LOG_STDOUT("SVT_TEST_CRITERIA: Passed av.testCriteriaPercentMessage\n");
        }
    }

    if (av.testCriteriaMsgCount_enabled) {
        // Criteria must be incremented for each enabled criteria
        svt_test_criteria++;
        if (( av.qos == 2 && av.testCriteriaMsgCount!=av.tv->rate_s.nmsgs) || 
            ( av.qos == 1 && (av.testCriteriaMsgCount>av.tv->rate_s.nmsgs)) ){  // not checked for Qos 0
            LOG_ERROR("SVT_TEST_CRITERIA: Failed av.testCriteriaMsgCount (expected %d messages) found count: %llu\n",
                av.testCriteriaMsgCount, av.tv->rate_s.nmsgs);
            svt_test_result++;
        }else {
            LOG_STDOUT("SVT_TEST_CRITERIA: Passed av.testCriteriaMsgCount\n");
        }
    } 

    if (av.testCriteriaOrderMsg_enabled) {
        // Criteria must be incremented for each enabled criteria
        svt_test_criteria++;
        if (av.testCriteriaOrderMsg>0){
            LOG_ERROR("SVT_TEST_CRITERIA: Failed av.testCriteriaOrderMsg (expected messages in order) out of order found count: %llu\n",
                av.testCriteriaOrderMsg);
            svt_test_result++;
        }else {
            LOG_STDOUT("SVT_TEST_CRITERIA: Passed av.testCriteriaOrderMsg\n");
        }
    } 

    if (av.testCriteriaOrderMin_enabled) {
        // Criteria must be incremented for each enabled criteria
        svt_test_criteria++;
        if (av.testCriteriaOrderMin>0){
            LOG_ERROR("SVT_TEST_CRITERIA: Failed av.testCriteriaOrderMin (expected message order to be at mininum above a certain range number) out of min range found count: %llu\n",
                av.testCriteriaOrderMin);
            svt_test_result++;
        }else {
            LOG_STDOUT("SVT_TEST_CRITERIA: Passed av.testCriteriaOrderMin\n");
        }
    }

    if (av.testCriteriaOrderMax_enabled) {
        // Criteria must be incremented for each enabled criteria
        svt_test_criteria++;
        if (av.testCriteriaOrderMax>0){
            LOG_ERROR("SVT_TEST_CRITERIA: Failed av.testCriteriaOrderMax (expected message order to be at mininum above a certain range number) out of max range found count: %llu\n",
                av.testCriteriaOrderMax);
            svt_test_result++;
        }else {
            LOG_STDOUT("SVT_TEST_CRITERIA: Passed av.testCriteriaOrderMax\n");
        }
    }


    if (av.testCriteriaVerifyMsg_enabled) {
        // Criteria must be incremented for each enabled criteria
        svt_test_criteria++;
        if (av.testCriteriaVerifyMsg>0){
            LOG_ERROR("SVT_TEST_CRITERIA: Failed av.testCriteriaVerifyMsg (expected messages to match specific pattern) verification failed count: %llu\n",
                av.testCriteriaVerifyMsg);
            svt_test_result++;
        } else {
            LOG_STDOUT("SVT_TEST_CRITERIA: Passed av.testCriteriaVerifyMsg\n");
        }
    } 

    if (av.halt != 0){
        LOG_STDOUT("NOTE: This was an abnormal exit via signal handler: av.halt: %d\n", av.halt );
    }


    if( (svt_test_result==0) && (svt_test_criteria > 0)){
        LOG_STDOUT("SVT_TEST_RESULT: SUCCESS ( %d of %d ) test criteria achieved\n", 
            svt_test_criteria, svt_test_criteria );
    } else if ((svt_test_criteria > 0) && (svt_test_result != 0)) {
        LOG_STDOUT("SVT_TEST_RESULT: FAILURE ( %d of %d ) test criteria achieved\n", 
            svt_test_criteria-svt_test_result, svt_test_criteria);
    } else {
        LOG_STDOUT("SVT_TEST_RESULT: SUCCESS - no specific criteria other than completion.\n");
    }


    if(av.msg_buf){
        free(av.msg_buf);
    }
    if(av.keyStore){
        free(av.keyStore);
    }
    if(av.trustStore){
        free(av.trustStore);
    }
    if(av.privateKey){
        free(av.privateKey);
    }
    if(av.privateKeyPassword){
        free(av.privateKeyPassword);
    }
    if(av.enabledCipherSuites){
        free(av.enabledCipherSuites);
    }
    if(av.verifyPubComplete){
        free(av.verifyPubComplete);
    }
    if (strcmp(av.action, SUBSCRIBE) == 0){
        do_shm_cleanup();
    }
    if (av.log2out) {
        if ( av.log2out!=stdout ){
             fclose(av.log2out);   
        }
    }
    free (av.tv);

    return 0;

}
