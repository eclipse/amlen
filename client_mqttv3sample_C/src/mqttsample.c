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

#include <mqttsample.h>

/*
 * @file mqttsample.c
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
 * -v indicates verbose output
 * 
 * -h output usage statement
 * 
 *
 */


/*
 * Initialize global variables
 */
MQTTClient g_mqtt_client;
MQTTClient_deliveryToken g_mqtt_token;
volatile int g_halt = 0;        
int g_qos=0;                   
int g_cleansession=1;
char g_uri[MAX_BUF_SZ]= {0};    
char g_user[MAX_BUF_SZ]= {0};    
char g_password[MAX_BUF_SZ]= {0};    
char g_topic[MAX_BUF_SZ]={0};   
char g_clientId[MAX_BUF_SZ]={0};
char g_action[MAX_BUF_SZ]={0};  
char defaultMsg[]="Message from MQTTv3 C client";
char *g_msg=NULL;               
char *g_msg_buf=NULL;           
char g_persistence_context_buf[MAX_BUF_SZ]={0}; 
char *g_persistence_context=NULL;
int g_appstate=STATE_INITIALIZING;
int g_persistence_type=MQTTCLIENT_PERSISTENCE_NONE;
int g_retained_flag=0;
unsigned int g_msg_number=1;    
unsigned int g_publish_count=0; 
unsigned int g_subscribe_count=0;
unsigned int g_warning_count=0;
unsigned int g_error_count=0;
unsigned int g_trace_level=0;   
FILE *g_log2out=NULL;          
FILE *g_log2err=NULL;         
int g_retry_connect=240; // arbitrary setting. 
rateControl g_rate_s={0,0,0,0,0,0,0,0,0.0,0.0,0.0};
rateControl *g_rate=NULL;
char *g_keyStore=NULL;
char *g_trustStore=NULL;
char *g_privateKey=NULL;
char *g_privateKeyPassword=NULL;
char *g_enabledCipherSuites=NULL;
int g_enableServerCertAuth=1;
unsigned long long g_lastStatTime=0;
int g_lastStatTimeIntervalUSec=60*MICROS_PER_SECOND;


int main(int argc, char **argv){
    int rc=0;
    time_t t_result; 
    char host[MAX_BUF_SZ]="";
    g_log2out=stdout; 
    g_log2err=stderr;
    signal(SIGUSR1, do_signal_handle);
    signal(SIGINT, do_signal_handle);
    signal(SIGTERM, do_signal_handle);

    g_lastStatTime=getHRTimer();

    /* Process user inputs */
    do_parse_arguments(argc, argv);
   
    if (g_clientId[0]=='\0'){ 
        /* Construct a default clientId if not specified */
        gethostname(host,MAX_BUF_SZ);
        time(&t_result);
        srand((unsigned int)t_result+getpid());
        sprintf(g_clientId, "%.1s%d%08x%s",g_action,getpid(),rand(),host);
    }
   
    /*
     * Per MQTT Spec, clientId must not be longer than 23 chars. 
     * Enforce the 23 char limit.
     */
    sprintf(g_clientId, "%.23s", g_clientId); 

    do_info_display_setup();

    /*
     * Run the requested user operation
     */
    if (strcmp(g_action, PUBLISH) == 0) {    
        rc=do_publish();
    } else if (strcmp(g_action, SUBSCRIBE) == 0) {
        rc=do_subscribe();
    } else {
        HALT_ERROR("Invalid action \"%s\" or no action specified.\n",g_action);
    }

    if (rc==0){
        g_appstate=STATE_SUCCESS;
    } else { 
        g_appstate=STATE_FAILURE;
    }

    do_info_display(MYDETAILED);
    
    LOG_INFO("%s rc=%d (%d)\n", g_action, rc, g_appstate);
    if(g_msg_buf){
        free(g_msg_buf);
    }
    if(g_keyStore){
        free(g_keyStore);
    }
    if(g_trustStore){
        free(g_trustStore);
    }
    if(g_privateKey){
        free(g_privateKey);
    }
    if(g_privateKeyPassword){
        free(g_privateKeyPassword);
    }
    if(g_enabledCipherSuites){
        free(g_enabledCipherSuites);
    }
    if ( g_log2out!=stdout ){
        fclose(g_log2out);
    }


    return 0;
}
