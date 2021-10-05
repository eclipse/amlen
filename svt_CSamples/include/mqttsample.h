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

#ifndef __MQTTSAMPLE_H__
#define __MQTTSAMPLE_H__

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
#include <time.h>
#include <sys/time.h>
#include <MQTTClient.h>
#include <MQTTClientPersistence.h>

extern char* getTimeString(unsigned long long, char*);

/*
 * Defines, Globals, and Macros
 */

#define MAX_BUF_SZ 1024             
#define PUBLISH "publish"
#define SUBSCRIBE "subscribe"       
#define KEEPALIVEINTERVAL_SECONDS 86400 /* Mqttclient library does not support setting this to 0*/
                                        /* for an "infinite" connection set to a large number*/
#define RELIABLE_FLAG_FALSE 0       /* allow up to 10 messages to be simulataneously in-flight*/
#define WAIT_FOR_COMPLETION_TIMEOUT 10000L
#define CLIENTID_MAX_LEN 23         /* MQTT v3.1 max for clientId is 23 chars*/
#define NANO_PER_MICRO 1000.0
#define MICROS_PER_SECOND 1000000.0

#define LOG_STDOUT( _fmt_, _args_...)  \
    { \
    char str[256]; \
    fprintf(g_log2out, "%s " _fmt_,  \
        getTimeString(getHRTimer(),str), \
        ##_args_); \
    } \
    fflush(g_log2out); \

#define LOG_INFO( _fmt_, _args_...)  \
    { \
    char str[256]; \
    if (g_trace_level>0){            \
        fprintf(g_log2out, "%s " _fmt_, \
        getTimeString(getHRTimer(),str), \
            ##_args_); \
    } \
    } \
    fflush(g_log2out);

#define LOG_WARNING( _fmt_, _args_...)  \
    { \
    char str[256]; \
    g_warning_count++; \
    fprintf(g_log2err, "%s WARNING: [%s:%d] errno:[%d]%s :" _fmt_, \
        getTimeString(getHRTimer(),str), \
       __FUNCTION__, __LINE__,errno, \
    errno ? strerror(errno): "" ,  \
    ##_args_); \
    } \
    fflush(g_log2err);

#define LOG_ERROR( _fmt_, _args_...)  \
    { \
    char str[256]; \
    g_error_count++; \
    fprintf(g_log2err, "%s ERROR: [%s:%d] errno:[%d]%s :" _fmt_, \
        getTimeString(getHRTimer(),str), \
       __FUNCTION__,__LINE__,errno, \
    errno ? strerror(errno): "" ,  \
    ##_args_);  \
    } \
    fflush(g_log2err);

#define HALT_ERROR( _fmt_, _args_...)  \
    { \
    char str[256]; \
    g_error_count++; \
    fprintf(g_log2err, "%s ERROR: HALT: [%s:%d] errno:[%d]%s :" _fmt_, \
        getTimeString(getHRTimer(),str), \
         __FUNCTION__,  __LINE__,errno, \
    errno ? strerror(errno): "" ,  \
    ##_args_); \
    } \
    exit(1);


typedef struct {
    int state;
    int desiredMsgsPerSec;
    unsigned long long nsleeps;
    unsigned long long nmsgs;
    unsigned long long firstMsgTime;       /* time first message published or received */
    unsigned long long appStartTime;    /* time that application started */
    unsigned long long warningTime;    /* time that last warning was reported */
    unsigned long long lastMsgTime;
    unsigned long long currTime;
    double elapsed;
    double projected;
    double sleepInterval;
} rateControl;

enum AppState {
    STATE_INITIALIZING,
    STATE_WAIT_TO_PUBLISH,
    STATE_CONTINUE,
    STATE_CONNECTING,
    STATE_CONNECTED,
    STATE_DISCONNECTED,
    STATE_SUBSCRIBED,
    STATE_RECEIVING,
    STATE_RECEIVED_ALL,
    STATE_PUBLISHING,
    STATE_PUBLISHED_ALL,
    STATE_SUCCESS,
    STATE_FAILURE
};

enum ConnectFlag {
    MYCONNECT,
    MYRECONNECT
};

enum DisplayFlag {
    MYBASIC,
    MYDETAILED
};

extern MQTTClient g_mqtt_client;
extern MQTTClient_deliveryToken g_mqtt_token;
extern rateControl g_rate_s;         /* storage for control the rate of messages*/
extern rateControl *g_rate;          /* control the rate of messages*/
extern int g_cleansession;          /* clean session boolean */
extern volatile int g_halt;         /* signal handling */
extern int g_qos;                   /* quality of service 0,1,or 2 */
extern int g_retry_connect;     
extern int g_appstate;
extern char g_uri[MAX_BUF_SZ];      /* URI to connect to format: "tcp://<ip>:<port>" */
extern char g_user[MAX_BUF_SZ];      /* URI to connect to format: "tcp://<ip>:<port>" */
extern char g_password[MAX_BUF_SZ];      /* URI to connect to format: "tcp://<ip>:<port>" */
extern char g_topic[MAX_BUF_SZ];    /* topic to publish or subscribe to*/
extern char g_clientId[MAX_BUF_SZ]; /* clientId for MQTTClient_create api <=23 chars*/
extern char g_action[MAX_BUF_SZ];   /* "publish" or "subscribe" */
extern char defaultMsg[MAX_BUF_SZ]; /* default msg if no user input for msg*/ 
extern char *g_msg;                 /* message to publish.(not used for subscribe)*/
extern char *g_msg_buf;             /* buffer for g_msg value*/
extern char *g_persistence_context; /* parameter for MQTTClient_create api */
extern char g_persistence_context_buf[MAX_BUF_SZ]; /*buffer to hold persistence value*/
extern int g_persistence_type;      /* parameter for MQTTClient_create api */
extern int g_retained_flag;         /* see RETAINED in MQTTv3.1 specification*/
extern unsigned long long g_msg_number;   /* the number of messages to publish or subscribe */
extern unsigned long long g_publish_count;/* the number of successful publishes*/   
extern unsigned long long g_subscribe_count;/* the number of successful subscribes*/ 
extern unsigned long long g_warning_count;
extern unsigned long long g_error_count;
extern unsigned int g_trace_level;  /* used for logging */
extern FILE *g_log2out;             /* used for logging */
extern FILE *g_log2err;             /* used for logging */
/*--------------------------------------------------------------------*/
/*-           SSL Changes            ---------------------------------*/
/*--------------------------------------------------------------------*/
extern char *g_keyStore;
extern char *g_trustStore;
extern char *g_privateKey;
extern char *g_privateKeyPassword;
extern char *g_enabledCipherSuites;
extern int g_enableServerCertAuth;
extern unsigned long long g_lastStatTime;
extern int g_lastStatTimeIntervalUSec;
extern unsigned long long g_lastActiveTime;
extern unsigned long long g_lastActiveCount;
extern unsigned long long g_verifyStillActive;


/*
 * Functions
 */

extern unsigned long long getHRTimer(void);

extern void do_signal_handle(int sig);
extern void do_status_update(void);

extern void do_usage(char *name);

extern void do_parse_arguments(int argc , char **argv);

extern int do_connect(MQTTClient * client, 
        const char * uri, 
        const char * clientId,
        const int persistence_type, 
        const char * persistence_context, 
        const int) ;
extern int do_reconnect(void);

extern int do_subscribe(void);

extern int do_publish(void);

extern int do_handle_mqtt_client_errors(int mqtt_rc,
                    unsigned int,
                    const char *,
                    const char *);
extern void do_rate_control(rateControl *r);
extern void do_info_display(int);
extern void do_info_display_setup(void);

#endif
