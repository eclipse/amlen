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
 * A sample mqtt tool
 * USAGE: ./mqtttool -h 
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <memory.h>
#include <ctype.h>
#include <MQTTClient.h>
#include <MQTTClientPersistence.h>

#if defined(WIN32)
#include <Windows.h>
#define sleep Sleep
#else
#include <sys/time.h>
#endif

#define _MULTI_THREADED
#include <pthread.h>

#define MAX_BUF_SZ 1024
#define PUBLISH "PUBLISH"
#define SUBSCRIBE "SUBSCRIBE"

int subscribe(char* uri, char* clientId, char* topicName, int qos);
int publish(char* uri, char* clientId, char* topicName, int qos, char* message);

volatile int g_halt = 0;
int g_qos=0;
char g_uri[MAX_BUF_SZ]="tcp://localhost:1883";
char g_topic[MAX_BUF_SZ]="";
char g_host[MAX_BUF_SZ]="";
char g_clientId[MAX_BUF_SZ]="";
char g_action[MAX_BUF_SZ]="";
char g_msg[MAX_BUF_SZ]="";
unsigned int g_msg_number=10; /* number of messages to send or receive before exitting */
unsigned int g_publish_count=0;    
unsigned int g_subscribe_count=0;    
unsigned int g_trace_level=0;
unsigned int g_timeout_seconds=0;
char g_status[MAX_BUF_SZ]="";
FILE *log2out;
FILE *log2err;

#define LOG_STDOUT( _fmt_, _args_...)  \
    fprintf(log2out, "LOG : [%s] " _fmt_, __FUNCTION__, ##_args_); 

#define LOG_STDERR( _fmt_, _args_...)  \
    fprintf(log2err, "LOG _ [%s] " _fmt_, __FUNCTION__, ##_args_); 

#define LOG_INFO( _fmt_, _args_...)  \
    if (g_trace_level>0){            \
         fprintf(log2err, "INFO: [%s] " _fmt_, __FUNCTION__, ##_args_); \
    } 

#define LOG_WARNING( _fmt_, _args_...)  \
    if (g_trace_level>0){            \
        fprintf(log2err, "WARNING: [%s] errno:[%d]" _fmt_, \
        	__FUNCTION__, errno, ##_args_); \
    } 

#define LOG_ERROR( _fmt_, _args_...)  \
    fprintf(log2err, "ERROR: [%s:%d] errno:[%d]" _fmt_, \
        	__FUNCTION__,__LINE__,errno, ##_args_);

#define HALT_ERROR( _fmt_, _args_...)  \
    fprintf(log2err, "ERROR: HALT: [%s:%d] errno:[%d]" _fmt_, __FUNCTION__, \
    	 __LINE__,errno, ##_args_); \
    exit(1);


void output_results(const char * msg){
    if (g_subscribe_count == g_msg_number || g_publish_count == g_msg_number) {
        LOG_STDOUT( "%s: Test MQTTv3 Success! ClientId: %s " \
                "Rx: %d Tx: %d\n",msg,g_clientId,
            g_subscribe_count,g_publish_count);    
    } else {
        LOG_ERROR( "%s: Test MQTTv3 Failure. " \
                " ClientId: %s Rx: %d Tx: %d\n",msg,g_clientId,
            g_subscribe_count,g_publish_count);    
    }
    
}

void handle(int sig){
    g_halt=1;
}

void usage(char *name){
    printf("\n");
    printf("------------------------------------------------------------\n");
    printf("Usage: %s [options] \n", name);
    printf("    Options: \n");
    printf("        -a action      : set the action (publish or subscribe)  \n");
    printf("           Valid actions : \n");
    printf("           -------------------------------------------------\n");
    printf("              publish  : configure tool as a publisher      \n");
    printf("              subscribe: configure tool as a subscriber     \n");
    printf("        -c clientid    : set the clientid \n");
    printf("        -h             : print help message\n");
    printf("        -m message     : set the message text      \n");
    printf("        -n number      : set the number of messages\n");
    printf("        -o logfile     : set the logfile for output \n");
    printf("        -q 0,1,2       : set the Quality of Service (0,1,or 2)\n");
    printf("        -t topic       : set the topic name\n");
    printf("        -u uri         : set the uri <tcp://ip:port>\n");
    printf("        -x seconds     : set the timeout seconds\n");
    printf("        -v             : set to verbose mode\n");
    printf("------------------------------------------------------------\n");
    printf("Notes:                                                      \n");
    printf("------------------------------------------------------------\n");
    printf("                                                            \n");
    printf("0. For verbose help with examples and more notes do -v -h   \n");
    printf("                                                            \n");
    if (g_trace_level>0){            \
    printf("------------------------------------------------------------\n");
    printf("Examples:                                                   \n");
    printf("------------------------------------------------------------\n");
    printf("                                                            \n");
    printf("Examples 1:Start a subscriber and publisher to send default msg\n");
    printf("           on default topic at default quality of service\n");
    printf("------------------------------------------------------------\n");
    printf("                                                            \n");
    printf("Start the subscriber:                                     \n");
    printf("                                                            \n");
    printf(" %s -u tcp://10.10.3.22:16102  -a subscribe \n",name);
    printf("                                                            \n");
    printf("Start the publisher:      \n");
    printf("                                                            \n");
    printf(" %s -u tcp://10.10.3.22:16102  -a publish\n",name);
    printf("                                                            \n");
    printf("------------------------------------------------------------\n");
    }
    exit(0);
}


void validate_arguments(void) {
    /* check that the input arguments are valid */
    return;
}

char * strtoupper(char*s){
    char *p=s;
    while(*p){*p++=toupper(*p);}
    return s;
}

void parse_arguments(int argc , char **argv){
    int c,i;
    if (argc<=1) { 
        usage(argv[0]); 
    }
    while ((c=getopt(argc,argv,"a:c:hm:n:o:q:t:u:vx:")) != -1) {
        switch (c) {
            case 'a':
                strncpy(g_action, optarg, MAX_BUF_SZ);
                strtoupper(g_action);
                LOG_INFO( "g_action is %s\n", g_action);
                if (g_topic[0] == '\0' ){
                    // Set the default topic according to the specified action
                    if (strcmp(g_action, PUBLISH) == 0){
                        strcpy(g_topic, "MQTTV3Sample/C/v3");
                    } else {
                        strcpy(g_topic, "MQTTV3Sample/#");
                    }
                }
                break;
            case 'c':
                strncpy(g_clientId, optarg, MAX_BUF_SZ);
                break;
            case 'h':
                usage(argv[0]);
                break;
            case 'm':
                strncpy(g_msg, optarg, MAX_BUF_SZ);
                break;
            case 'n':
                g_msg_number=strtoul(optarg,0,10);
                LOG_INFO( "g_msg_number is %d\n", g_msg_number);
                break;
            case 'o':
                log2out=fopen(optarg, "w+");
                if (!log2out) {
                      HALT_ERROR ("Unable to open output log file %s\n", optarg);
                }
                log2err=log2out;
                break;
            case 'q':
                g_qos=strtoul(optarg,0,10);
                if (g_qos>2 || g_qos<0) { HALT_ERROR ("Invalid Qos %d\n", g_qos); }
                break;
            case 't':
                strncpy(g_topic, optarg, MAX_BUF_SZ);
                break;
            case 'u':
                strncpy(g_uri, optarg, MAX_BUF_SZ);
                break;
            case 'v':
                g_trace_level=10;
                break;
            case 'x':
                g_timeout_seconds=strtoul(optarg,0,10);
                break;
            default:
                 LOG_ERROR("Incorrect switch.\n");
                usage(argv[0]);
        }
    }

    for (i=optind; i< argc; i++) {
          HALT_ERROR ("Unexpected non-option argument number %d \"%s\".\n"\
                "Check help menu with -h\n", i, argv[i]);
    }

}

/**
 * Subscribes to a topic and blocks until Ctrl-C is pressed,
 * or the connection is lost.
 *
 * The sample demonstrates synchronous subscription, which
 * does not use callbacks.
 */
int subscribe(char* uri, char* clientId, char* topicName, int qos) {
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message* message = NULL;
    int rc;
    char* receivedTopicName = NULL;
    long receiveTimeout = 100l;
    int topicLen;

    // Create the client instance
    rc = MQTTClient_create(&client, uri, clientId, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTCLIENT_SUCCESS) {
        LOG_ERROR("Failed to create MQTT Client instance\n");
        return rc;
    }

    // Create the connection options
    conn_opts.keepAliveInterval = 20;
    conn_opts.reliable = 0;
    conn_opts.cleansession = 1;

    // Connect the client
    rc = MQTTClient_connect(client, &conn_opts);
    if (rc != MQTTCLIENT_SUCCESS) {
        LOG_ERROR("Failed to connect, return code %d\n", rc);
        MQTTClient_destroy(&client);
        return rc;
    }

    LOG_INFO("Connected to %s\n",uri);

    // Subscribe to the topic
    LOG_INFO("Subscribing to topic \"%s\" qos %d\n", topicName, qos);

    rc = MQTTClient_subscribe(client, topicName, qos);

    if (rc != MQTTCLIENT_SUCCESS) {
        LOG_WARNING("Failed to subscribe, return code %d\n", rc);
    } else {
        while (!g_halt && (g_subscribe_count<g_msg_number)) {
            receivedTopicName = NULL;
            topicLen=0;

            // Check to see if a message is available
            rc = MQTTClient_receive(client, &receivedTopicName, &topicLen, &message, receiveTimeout);

            if (rc == MQTTCLIENT_SUCCESS && message != NULL) {
                // A message has been received
                LOG_INFO("Topic:\t\t%s\n", receivedTopicName);
                LOG_INFO("Message:\t%.*s\n", message->payloadlen, (char*)message->payload);
                LOG_INFO("QoS:\t\t%d\n", message->qos);
                if (g_msg[0] != '\0' ) { /* user wants it validated too. */
                	if(strncmp((char*)message->payload,g_msg,message->payloadlen)!= 0){
						HALT_ERROR("Failed to validate message\n");
					}
				}
                g_subscribe_count++;
                MQTTClient_freeMessage(&message);
                MQTTClient_free(receivedTopicName);
            } else if (rc != MQTTCLIENT_SUCCESS) {
                LOG_ERROR("Failed to received message, return code %d\n", rc);
                g_halt = 1;
            } else {
                //LOG_INFO("Continuing\n");
            }
        }
    }

    // Disconnect the client
    MQTTClient_disconnect(client, 0);

    LOG_INFO("Disconnected\n");

    // Free the memory used by the client instance
    MQTTClient_destroy(&client);

    return rc;
}

int publish(char* uri, char* clientId, char* topicName, int qos, char* message) {
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message mqttMessage = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;

    // Create the client instance
    rc = MQTTClient_create(&client, uri, clientId, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTCLIENT_SUCCESS) {
        LOG_ERROR("Failed to create MQTT Client instance\n");
        return rc;
    }

    // Create the connection options
    conn_opts.keepAliveInterval = 20;
    conn_opts.reliable = 0;
    conn_opts.cleansession = 1;

    // Connect the client
    rc = MQTTClient_connect(client, &conn_opts);
    if (rc != MQTTCLIENT_SUCCESS) {
        LOG_ERROR("Failed to connect, return code %d\n", rc);
        MQTTClient_destroy(&client);
        return rc;
    }

    LOG_INFO("Connected to %s\n",uri);

    mqttMessage.payload = message;
    mqttMessage.payloadlen = strlen(message);
    mqttMessage.qos = qos;
    mqttMessage.retained = 0;

    while(!g_halt && (g_publish_count < g_msg_number)) {
    
        // Publish the message
        LOG_INFO("Publishing to topic \"%s\" qos %d\n", topicName, qos);
    
        rc = MQTTClient_publishMessage(client, topicName, &mqttMessage, &token);
    
        if ( rc != MQTTCLIENT_SUCCESS ){
            printf("Message not accepted for delivery, return code %d\n", rc);
        } else {
            // Wait until the message has been delivered to the server
            rc = MQTTClient_waitForCompletion(client, token, 10000L);
    
            if ( rc != MQTTCLIENT_SUCCESS ){
                LOG_WARNING("WARNING:Failed to deliver message, return code %d\n",rc);
            }
        }
        g_publish_count++;
    }

    // Disconnect the client
    MQTTClient_disconnect(client, 0);

    LOG_INFO("Disconnected\n");

    // Free the memory used by the client instance
    MQTTClient_destroy(&client);

    return rc;
}

void start_publisher(void){
    int rc;
    rc=publish(g_uri, g_clientId, g_topic,g_qos,g_msg);
    LOG_INFO("publish rc: %d\n", rc);
}


void start_subscriber(void){
    int rc;
    rc=subscribe(g_uri, g_clientId, g_topic, g_qos);
    LOG_INFO("subscribe rc: %d\n", rc);
}


int main(int argc, char **argv){
    log2out=stdout;
    log2err=stderr;
    signal(SIGALRM, handle);
    signal(SIGINT, handle);
    signal(SIGTERM, handle);

    parse_arguments(argc, argv);
    
    if (g_timeout_seconds>0){
        alarm(g_timeout_seconds);
    }

    // Construct the full broker URL and clientId
    gethostname(g_host,MAX_BUF_SZ);
	sprintf(g_clientId, "%s_%s_%d_%s", g_clientId,g_action,getpid(),g_host);

    if (strcmp(g_action, PUBLISH) == 0) {    /* note if not really necessary, but validates inputs */
    	start_publisher();
    } else if (strcmp(g_action, SUBSCRIBE) == 0) {
        start_subscriber();
    } else {
        HALT_ERROR("Invalid action %s\n",g_action);
    }

    output_results(__FUNCTION__);
    return 0;
}
