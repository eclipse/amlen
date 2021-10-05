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
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <memory.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <fcntl.h>           /* For O_* constants */
#include <semaphore.h>
#include <MQTTClient.h>
#include <MQTTClientPersistence.h>



//#define _MULTI_THREADED
//#include <pthread.h>


extern char* getTimeString(unsigned long long, char*);

/*
 * Defines, Globals, and Macros
 */

#define MAX_BUF_SZ 1024             
#define PUBLISH "publish"
#define PUBLISH_WAIT "publish_wait"
#define SUBSCRIBE "subscribe"       
#define RECEIVE_WAIT "receive_wait"
#define KEEPALIVEINTERVAL_SECONDS 86400 /* Mqttclient library does not support setting this to 0*/
                                        /* for an "infinite" connection set to a large number*/
#define RELIABLE_FLAG_FALSE 0       /* allow up to 10 messages to be simulataneously in-flight*/
#define WAIT_FOR_COMPLETION_TIMEOUT 10000L
#define CLIENTID_MAX_LEN 23         /* MQTT v3.1 max for clientId is 23 chars*/
#define NANO_PER_MICRO 1000.0
#define MICROS_PER_SECOND 1000000.0

#define LOG_STDOUT_SYNC( _fmt_, _args_...)  \
    { \
    char str[256]; \
    fprintf(av.log2out, "%s " _fmt_,  \
        getTimeString(getHRTimer(),str), \
        ##_args_); \
    } \
    fflush(av.log2out); \
    fsync(fileno(av.log2out)); \

#define LOG_STDOUT( _fmt_, _args_...)  \
    { \
    char str[256]; \
    fprintf(av.log2out, "%s " _fmt_,  \
        getTimeString(getHRTimer(),str), \
        ##_args_); \
    } \
    fflush(av.log2out); \

#define LOG_STDOUT_NO_TIMESTAMP( _fmt_, _args_...)  \
    { \
    fprintf(av.log2out, _fmt_,  \
        ##_args_); \
    } \
    fflush(av.log2out); \

#define CLIENT_LOG_STDOUT(tv, _fmt_, _args_...)  \
    { \
    char str[256]; \
    fprintf(av.log2out, "%s %d: " _fmt_,  \
        getTimeString(getHRTimer(),str), \
        tv->array_id, \
        ##_args_); \
    } \
    fflush(av.log2out); \


#define LOG_INFO( _fmt_, _args_...)  \
    { \
    char str[256]; \
    if (av.trace_level>0){            \
        fprintf(av.log2out, "%s " _fmt_, \
        getTimeString(getHRTimer(),str), \
            ##_args_); \
    } \
    } \
    fflush(av.log2out);

#define CLIENT_LOG_INFO(tv, _fmt_, _args_...)  \
    { \
    char str[256]; \
    if (av.trace_level>0){            \
        fprintf(av.log2out, "%s %d: " _fmt_, \
        getTimeString(getHRTimer(),str), \
        tv->array_id, \
            ##_args_); \
    } \
    } \
    fflush(av.log2out);

#define LOG_WARNING( _fmt_, _args_...)  \
    { \
    char str[256]; \
    av.warning_count++; \
    fprintf(av.log2err, "%s WARNING: [%s:%d] errno:[%d]%s :" _fmt_, \
        getTimeString(getHRTimer(),str), \
       __FUNCTION__, __LINE__,errno, \
    errno ? strerror(errno): "" ,  \
    ##_args_); \
    } \
    fflush(av.log2err);

#define CLIENT_LOG_WARNING( tv, _fmt_, _args_...)  \
    { \
    char str[256]; \
    av.warning_count++; \
    fprintf(av.log2err, "%s WARNING: %d [%s:%d] errno:[%d]%s :" _fmt_, \
        getTimeString(getHRTimer(),str), \
        tv->array_id, \
       __FUNCTION__, __LINE__,errno, \
    errno ? strerror(errno): "" ,  \
    ##_args_); \
    } \
    fflush(av.log2err);


#define LOG_ERROR( _fmt_, _args_...)  \
    { \
    char str[256]; \
    av.error_count++; \
    fprintf(av.log2err, "%s ERROR: [%s:%d] errno:[%d]%s :" _fmt_, \
        getTimeString(getHRTimer(),str), \
       __FUNCTION__,__LINE__,errno, \
    errno ? strerror(errno): "" ,  \
    ##_args_);  \
    } \
    fflush(av.log2err);

#define CLIENT_LOG_ERROR(tv, _fmt_, _args_...)  \
    { \
    char str[256]; \
    av.error_count++; \
    tv->error_count++; \
    fprintf(av.log2err, "%s ERROR: %d [%s:%d] errno:[%d]%s :" _fmt_, \
        getTimeString(getHRTimer(),str), \
        tv->array_id, \
       __FUNCTION__,__LINE__,errno, \
    errno ? strerror(errno): "" ,  \
    ##_args_);  \
    } \
    fflush(av.log2err);


#define HALT_ERROR( _fmt_, _args_...)  \
    { \
    char str[256]; \
    av.error_count++; \
    fprintf(av.log2err, "%s ERROR: HALT: [%s:%d] errno:[%d]%s :" _fmt_, \
        getTimeString(getHRTimer(),str), \
         __FUNCTION__,  __LINE__,errno, \
    errno ? strerror(errno): "" ,  \
    ##_args_); \
    if (strcmp(av.action, SUBSCRIBE) == 0){ \
        do_shm_cleanup(); \
    } \
    } \
    exit(1);


typedef struct {
    int state;
    int desiredMsgsPerSec;
    unsigned long long lastSampleMsgs;
    unsigned long long lastSampleTime;
    unsigned long long nsleeps;
    unsigned long long nmsgs;
    unsigned long long currentnmsgs;     /* needed for dynamic publishing rate*/
    unsigned long long firstMsgTime;       /* time first message published or received */
    unsigned long long currentfirstMsgTime;       /* needed for dynamic publishing rate*/
    unsigned long long appStartTime;    /* time that application started */
    unsigned long long warningTime;    /* time that last warning was reported */
    unsigned long long lastMsgTime;
    unsigned long long currentlastMsgTime;      /* needed for dynamic publishing rate*/
    unsigned long long currTime;
    double elapsed;
    double projected;
    double sleepInterval;
} rateControl;


enum ClientState {
    STATE_INITIALIZING,
    STATE_FAILED_CONNECTION,
    STATE_FAILOVER,  
    STATE_DISCONNECTING,
    STATE_DISCONNECTED,
    STATE_CONTINUE,
    STATE_CONNECTING,
    // -----------------------------------
    // IMPORTANT: Below STATE_CONNECTED should only include STATEs that imply connected 
    // as well, so that >= checks can be made.
    // -----------------------------------
    STATE_CONNECTED,
    STATE_WAIT_TO_PUBLISH,
    STATE_WAIT_TO_RECEIVE,
    STATE_SUBSCRIBED,
    STATE_RECEIVING,
    STATE_PUBLISHING,
    // -----------------------------------
    // IMPORTANT: Below STATE_COMPLETE should not be reconnected in the event of a failure.
    // -----------------------------------
    STATE_COMPLETE, 
    STATE_RECEIVED_ALL,
    STATE_WAIT_AFTER_RECEIVE,
    STATE_PUBLISHED_ALL,
};

enum DisplayFlag {
    MYBASIC,       // display an update
    MYDETAILED
};


enum ConnectFlag {
    MYCONNECT,
    MYRECONNECT
};

#define SHM_MAX_LEN 32

struct shm_rw_region {        /* Defines read/writable "structure" of shared memory */
    unsigned long long rate;   
    unsigned long long count; 
};

struct shm_region {        /* Defines "structure" of shared memory */
    int reserved;
    struct shm_rw_region subrw;  /* read/write area 0*/
    int reserved2;
    struct shm_rw_region pubrw;  /* read/write area 1*/
    unsigned int initialized; /* write 0xFABEBABE when intialized*/
    sem_t semaphore;  
};



typedef struct {
    MQTTClient mqtt_client;   // the "active" mqtt client.
    MQTTClient primary_mqtt_client; // the default mqtt client
    MQTTClient backup_mqtt_client;  // for backup mqtt client for high availability
    MQTTClient_deliveryToken mqtt_token;
    rateControl rate_s;             /* storage for control the rate of messages*/
    rateControl *rate;              /* control the rate of messages*/
    int retry_connect;              /* */
    int clientstate;
    int array_id;
    int total_clients;
    unsigned long long publish_count;     /* the number of successful publishes*/   
    unsigned long long subscribe_count;   /* the number of successful subscribes*/ 
    unsigned long long warning_count;
    unsigned long long error_count;
    long long orderMsgLastNumber;
    char * clientMsgBuf;
    MQTTClient_message mqttMessage;
    char topic[MAX_BUF_SZ];
    // Note about topic prepends:
    //      These topic prepends will always be prepended in this order currently
    //          /CommonName/UserXXXXX  - if this order is not desired further code changes will be needed.
    //      Above XXXX is the rest of the topic that normally would be used. 
    char topicCNPrepend[11];       // /CN0999999 10 max supported (common name prepend)
    char topicUSPrepend[11];       // /u0999999 9 max supported (user prepend)
    char clientId[24]; /* clientId for MQTTClient_create api <=23 chars*/
    int connectionCounter;  // count the number of times the client was connected. Best is 1.
    int connectionFailure;  // count the number of times the client failed to connect. Best is 0.
    int connectionCounterLastPublish;  // Used for RTC 28794 - workaround, republish message if ha connection to new server
    char * curURI;   // Used for RTC 28794 - workaround, republish message if ha connection to new server
    char * lastActionURI;   // Used for speedy failover detection and recovery RTC 25776
    char * lastPublishURI;   // Used for RTC 28794 - workaround, republish message if ha connection to new server
    unsigned long long receiveTimeoutExpired;
    char * ltpaToken; // unused unless using  av.userLtpaFile
    unsigned long long subscribeTime; //  when did subscribe complete.

} clientVariables;

typedef struct {
    volatile int halt;         /* signal handling */
    unsigned int trace_level;  /* used for logging */
    FILE *log2out;             /* used for logging */
    FILE *log2err;             /* used for logging */
    int cleansession;          /* clean session boolean */
    int qos;                   /* quality of service 0,1,or 2 */
    char haURI[MAX_BUF_SZ];    /* URI for highAvailability to connect to format: "tcp://<ip>:<port>" or.. ssl*/
    char uri[MAX_BUF_SZ];      /* URI to connect to format: "tcp://<ip>:<port>" or ssl://<ip>:<port> */
    char user[MAX_BUF_SZ];     /* */
    char password[MAX_BUF_SZ]; /* */
    char topic[MAX_BUF_SZ];    /* topic to publish or subscribe to*/
    char clientId[MAX_BUF_SZ]; /* clientId for MQTTClient_create api <=23 chars*/
    char action[MAX_BUF_SZ];   /* "publish" or "subscribe" */
    char defaultMsg[MAX_BUF_SZ]; /* default msg if no user input for msg*/ 
    char *msg;                 /* message to publish.(not used for subscribe)*/
    char *msg_buf;             /* buffer for g_msg value*/
    char *persistence_context; /* parameter for MQTTClient_create api */
    char persistence_context_buf[MAX_BUF_SZ]; /*buffer to hold persistence value*/
    int persistence_type;      /* parameter for MQTTClient_create api */
    int retained_flag;         /* see RETAINED in MQTTv3.1 specification*/
    int desiredMsgsPerSec;
    unsigned long long msg_number;        /* the number of messages to publish or subscribe */
    unsigned long long warning_count;
    unsigned long long error_count;
    volatile int info;
    int num_clients;
    clientVariables *tv;
    int appstate;
/*--------------------------------------------------------------------*/
/*-           SSL Changes            ---------------------------------*/
/*--------------------------------------------------------------------*/
    char *keyStore;             
    char *trustStore;             
    char *privateKey;             
    char *privateKeyPassword;             
    char *enabledCipherSuites;
    int enableServerCertAuth;
    int scaleTest;
    int orderMsg;
    unsigned long long lastScaleTestAlertTime;
    unsigned long long lastStatTime;
    unsigned long long lastStatMsgs;
    int lastStatTimeIntervalUSec;
    unsigned long long lastActiveTime;
    unsigned long long lastActiveCount;
    unsigned long long verifyStillActive;
    int sendOutOfOrderMsg;
    int topicChangeTest;
    int eightySixWaitForCompletion;
    int noDisconnect;
    int noError;
    int testCriteriaPercentMessage;
    int testCriteriaPercentMessage_enabled;
    int reconnectWait;
    int connectTimeout; // default to -1 means not enabled
    int userReceiveTimeout; //default to -1 means not enabled
    int haConnect; // if haURI is set this value toggles between 1 and 2 and will be set to 1 
                   // when it is 1 it connects to the uri, 
                   // when it is 2 it connects to haURI
    int retry_connect; // this value is passed along to all clients
    int verifyMsg; 
    int msg_buf_len;
    int sleepLoop;
    clientVariables **tv_array;
    int retainedMsgCounted; // whether or not subscriber should "count" retained messages default = no
    int keepAliveInterval;
    int reliable;
#if 0
    int messagePattern;  // 1 - Fan out per device request-reply One publisher device publishes messages 
                         // to many topic strings. Each message has only one subscriber device. Each 
                         // subscriber device publishes reply messages to the publisher device.
                         //
    int messagePatternArg1;  // When messaging patterns are used, other optional args may be needed
#endif
    int destroyClientOnAllConnectFails;
    int haHint;
    int subscribeOnReconnect;
    int testCriteriaOrderMsg_enabled;
    unsigned long long testCriteriaOrderMsg;
    int testCriteriaVerifyMsg_enabled;
    unsigned long long  testCriteriaVerifyMsg;
    int publishDelayOnConnect;
    int userIncrementer;  // default is -1 (not enabled)
    unsigned long long connectionAttempts;
    unsigned long long connectionSumTime;
    unsigned long long connectionMinTime;
    unsigned long long connectionMaxTime;
    unsigned long long connectionCounter;
    unsigned long long connectionFailure;
    int desiredWaitTime; // wait > 1 second per message sent/received
    int desiredMsgBurst; // do message bursts of this count before executing desiredWaitTime
    int waitForCompletionMode;  // default - 0 wait after each publish, 1 - process in batches
    int warningWait; // Default wait 1 seconds after a warning to slow down warning times
    int testCriteriaMsgCount;
    int testCriteriaMsgCount_enabled;
    char *verifyPubComplete;
    int dynamicReceiveTimeout;
    int checkConnection;
    int checkConnectionCounter;
    int badTopicChangeTest;
    int subscribeOnConnect;
    int warningWaitDynamic; // Default disabled - set to 1 to cause warningWait to go to 1 after a failed connection, and back to 0 after a successful connections
    int topicMultiTest;
    unsigned long long topicMultiTestCounter;
    int incrPubRate;                        // DEPRECATED 
    int incrPubRateInterval;                // DEPRECATED
    int incrPubRateCount;                   // DEPRECATED
    unsigned long long incrPubRateNextTime; // DEPRECATED
    int incrPubFeedback;                    // DEPRECATED
    int incrPubFeedbackDirection;           // DEPRECATED
    double incrPubFeedbackLastRate;         // DEPRECATED
    char userLtpaFile[MAX_BUF_SZ];   
    char publishThrottleFile[MAX_BUF_SZ];   // DEPRECATED
    int publishThrottleState;               // DEPRECATED
    int publishThrottleHighWaterMark;       // DEPRECATED
    int publishThrottleHighWaterMarkReached;// DEPRECATED
    int activeSleep;    // initial value of 100000 usec, sleeps until messaging is active. Max value of 1000000
    int activeCount;    // initial value of 0, after every 1000 it increments activeSleep by 1000
    int activeLastMsgCount;  // initial value of 0, after every 1000 message attempts with 0 successful throughput, increments activeSleep by 1000

    char dynamicClientCert[MAX_BUF_SZ];
    int dccIncrementer ;
    double connectRate;
    int prependCN2Topic;
    int useCNasClientID;
    int useWrongCNasClientID;
    char stopClientFile[MAX_BUF_SZ];
    int destroyClientBeforeConnect;
    struct shm_region *shm_ptr;
    int shmfd;
    char sharedMemoryKey[MAX_BUF_SZ];
    long long sharedMemoryVal;
    int sharedMemoryInit;
    int sharedMemoryType;
    int sharedMemoryCoupling;
    char throttleFile[MAX_BUF_SZ];
    time_t throttleFileLastModTime;
    unsigned long long testCriteriaOrderMinVal;
    unsigned long long testCriteriaOrderMin;
    int testCriteriaOrderMin_enabled;
    unsigned long long testCriteriaOrderMaxVal;
    unsigned long long testCriteriaOrderMax;
    int testCriteriaOrderMax_enabled;
    unsigned long long orderMsgStart;
    int unsubscribe;
    int usleepLoop;
    int dynamicReceiveTimeoutv2;
    char sharedSubscription[MAX_BUF_SZ];
    int sharedSubMulti;
    unsigned long long sharedSubMultiCounter;
    unsigned long long topicMultiTestIncr;
    int injectDisconnect;
    int injectDisconnectX;
    int injectDisconnectMax;
    int injectDisconnectCount;
    int skipSubscribe;
    int waitAfterReceive;
    int trueSleepAfterConnect;
    int topic_buf_len;
    char * topic_buf;
    char * topic_buf_ptr;
    int topic_buf_plen;
    int unsubscribeAfterRecv;
    int subscribeAfterRecv;
    int unsubscribeAfterConn;
    int msgTimeoutAfterSub;

    char * willTopic;
    char * willMessage;
    int willRetained;
    int willQos;


} appVariables;


extern appVariables av;



/*
 * Functions
 */

extern void do_load_file(char *inputfile, char **buf, int *len  );
extern int get_rand_chance(int chance_0_to_x, int x);
extern int do_shm_cleanup(void);
extern int do_shm_setup(void);
extern void do_shm_read(struct shm_rw_region *v, int area);
extern void do_shm_write(struct shm_rw_region *v, int area);

extern void do_disconnect_and_destroy_client(clientVariables *tv);
extern char * get_next_ltpa_token (clientVariables *tv);
extern unsigned long long getHRTimer(void);
extern void do_signal_handle(int sig);
extern void do_usage(char *name);
extern void do_parse_arguments(int argc , char **argv);
extern int do_connect(clientVariables*, int flag);
extern int do_connect_array(clientVariables**);
extern int do_reconnect(clientVariables*);
extern void* do_subscribe(void*);
extern void* do_publish(void*);
extern int do_handle_mqtt_client_errors(clientVariables*,
                    int mqtt_rc,
                    unsigned int,
                    const char *,
                    const char *);
extern int do_set_msg_rate(char *input, int flag);
extern void do_subscriber_throttle_check(void);
extern void do_rate_control(clientVariables*,int);
extern void do_rate_control_v2(clientVariables*,int);
extern void do_info_display(clientVariables*,int,int);
extern void do_info_display_setup(clientVariables*);
extern void do_status_update(clientVariables *tv,int);
extern void do_disconnect_and_destroy_array(clientVariables **tv);
extern int do_single_subsription( clientVariables *tv );
extern void do_safe_mqtt_client_sleep(clientVariables *tv, int seconds) ;
extern void do_safe_mqtt_client_usleep(clientVariables *tv, long long useconds) ;
extern int do_check_array_connections(clientVariables** tv, char*);
extern int do_check_file_exists(char *filename);
extern void do_set_client_topic(clientVariables *tv, const char * pub_or_sub);
extern void do_throttle_check(void);

extern int wrap_MQTTClient_receive(MQTTClient h, char** tn, int* tl, MQTTClient_message **m, unsigned long to);
extern int wrap_MQTTClient_subscribe(MQTTClient h, char* t, int q);
extern void wrap_MQTTClient_freeMessage(MQTTClient_message **m);
extern void wrap_MQTTClient_free(void *v);
extern int wrap_MQTTClient_create (MQTTClient *h, char *s, char *c, int p, void *pc);
extern int wrap_MQTTClient_connect (MQTTClient h, MQTTClient_connectOptions *o);
extern int wrap_MQTTClient_disconnect (MQTTClient h, int t);
extern int wrap_MQTTClient_isConnected (MQTTClient h);
extern void wrap_MQTTClient_destroy (MQTTClient *h);
extern int wrap_MQTTClient_publishMessage (MQTTClient h, char *t, MQTTClient_message *m, MQTTClient_deliveryToken *d) ;
extern int wrap_MQTTClient_waitForCompletion (MQTTClient h, MQTTClient_deliveryToken d, unsigned long t) ;
extern int wrap_MQTTClient_unsubscribe(MQTTClient h, char* t);
extern void do_circular_read_line_from_buf(char *buf, char **buf_ptr, int *out_ptr_len);





#define MY_ARRAY_INIT_VARIABLES int bad_count=0, good_count=0, i=0;
#define MY_ARRAY_CHECK_X(a,b,c) { good_count=0; \
    for(i=0; i<tv[0]->total_clients; i++){ \
            if(c==0 && tv[i]->clientstate==(a)){ \
                good_count++; \
            } else if(c==3 && tv[i]->clientstate>=(a)){ \
                good_count++; \
            } else if(c==1 && tv[i]->subscribe_count>=(a)){ \
                good_count++; \
                tv[i]->clientstate=STATE_RECEIVED_ALL; \
            } else if(c==2 && tv[i]->publish_count>=(a)){ \
                good_count++; \
                tv[i]->clientstate=STATE_PUBLISHED_ALL; \
            } else if (c<0 || c>3) {\
                HALT_ERROR("HALT - code bug invalid c parm to macro: %d",c); \
            } \
    } \
    if (good_count == tv[0]->total_clients) { \
        LOG_STDOUT_SYNC("Success - %d All Clients " b "\n", tv[0]->total_clients); \
        rc=0; \
        break; \
    }\
    }

#define MY_ARRAY_CHECK_CLIENT_STATE_EQ(a,b)  MY_ARRAY_CHECK_X(a,b,0);
#define MY_ARRAY_CHECK_CLIENT_STATE_GREATER_THAN_OR_EQ(a,b)  MY_ARRAY_CHECK_X(a,b,3);
#define MY_ARRAY_CHECK_MESSAGES_RECEIVED(a,b)  MY_ARRAY_CHECK_X(a,b,1);
#define MY_ARRAY_CHECK_MESSAGES_PUBLISHED(a,b)  MY_ARRAY_CHECK_X(a,b,2);

// - changed on 3.7.13 - from total_clients to 1. For connect_array it was going into a loop.
//            if (bad_count >= tv[0]->total_clients) { 
// - changed it back on 3.22.13 - but i get bad count sometimes and things stop ... i think i want a retry...
// took out the continue in the bad_count block... maybe that will fix it. also took out bad_count=0 else clause
#define MY_ARRAY_CHECK_ERROR_LOOP_START   \
    good_count=0; \
    bad_count=0; \
    for(i=0; i<tv[0]->total_clients; i++){ \
        if (av.halt) { LOG_INFO("av.halt is set!!! "); break; } \
        if (tv[i]->error_count!=0){ \
            bad_count++; \
            if (bad_count >= tv[0]->total_clients) { \
                LOG_ERROR("Set halt - exceeded bad_count: %d\n", bad_count); \
                av.halt=1; \
                break; \
            }    \
        } else if (tv[i]->clientstate==STATE_RECEIVED_ALL) { \
            continue; \
        } else if (tv[i]->clientstate==STATE_PUBLISHED_ALL) { \
            continue; \
        }

        //} else { 
            //bad_count=0;
        //}

#define MY_ARRAY_CHECK_ERROR_LOOP_END } \
    LOG_STDOUT_SYNC("Array check error loop end - good : %d, bad :%d \n", good_count , bad_count);


// User probably needs to reconnect if either of the below MACROS return 911.
#define MQTTCLIENT_NULL_CHECK if (h == NULL) { return 911; } 
#define MQTTCLIENT_CONNECTED_CHECK if (h == NULL) { return 911; }

#define MQTTCLIENT_ERROR_INJECT_DISCONNECT(h) if (av.injectDisconnect > 0 ) { \
    if ((av.injectDisconnectMax >0  && av.injectDisconnectCount < av.injectDisconnectMax ) ||  \
        av.injectDisconnectMax==0 ) { \
        if ( get_rand_chance(av.injectDisconnect, av.injectDisconnectX)) { \
            av.injectDisconnectCount++; \
            wrap_MQTTClient_disconnect (h, 0); \
        } \
    } \
}


#endif
