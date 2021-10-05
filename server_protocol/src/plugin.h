/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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

#ifndef PLUGINH
#define PLUGINH
#include <protocol.h>
#include <engine.h>

/*
 * These must match the values in Java
 */
enum PluginAction_e {
    /* Server to plug-in - global  */
    /* Sent to plugin as first message on the control channel, expects sync Reply */
    PluginAction_Initialize      = 1,   /* h0=id p=config */

    /* Sent to plugin when messaging is active and after all init is done, expects sync Reply */
    PluginAction_StartMessaging  = 2,   /* h0=role */

    /* Sent to plugin when the server is terminating. expects sync Reply */
    PluginAction_Terminate       = 3,   /* h0=rc, h1=message */

    /* Sent to the plugin during init, expects sync Reply */
    PluginAction_DefinePlugin    = 4,   /* p=properties */

    /* Sent to the plugin during inti, expects sync Reply */
    PluginAction_Endpoint        = 5,   /* p=properties */

    /* Sent to the plugin for each IOP, expects sync Reply */
    PluginAction_InitChannel     = 6,   /* h0=id */

    /* Sent to the plugin to authenticate a connection, expects AsyncReply */
    PluginAction_Authenticate    = 10,   /* h0=connect, h1=seqnum, p=props */

    PluginAction_ConfigChange    = 12,   /* p=config */

    /* Sent to the plugin to request statistics */
    PluginAction_GetStats        = 13,   /* h0=type */

    /* Incoming plug-in connection to server */
    PluginAction_InitConnection    = 0x0F, /* h0=id Do not change this value */

    /* Sent to plugin to check a connection, expects Accept */
    PluginAction_OnConnection    = 20,   /* h0=connect h1=seqnum h2=type ?h3=newconnect, b=bytes */

    /* Sent to plugin to indicate a connection is closing, expects AsyncReply */
    PluginAction_OnClose         = 21,   /* h0=connect h1=seqnum h2=rc, h3=reason */

    /* Sent to plugin to indicate completion of a request, no reply */
    PluginAction_OnComplete      = 22,   /* h0=connect h1=seqnum h2=rc h3=data */

    /* Sent to plugin with data from a connection, no Reply */
    PluginAction_OnData          = 23,   /* h0=connect b=bytes */

    /* Sent to plugin with a message to deliver, expects AsyncReply */
    PluginAction_OnMessage       = 24,   /* h0=connect h1=seqnum, h2=msgtype, h3=flags, h4=subname, h5=dest, p=properties, b=body */

    /* Sent to plugin to indicate completion of the setIdentity request. Plug-in can close connection if authorization failed. */
    PluginAction_OnConnected     = 26,   /* h0=connect, h1=seqnum, h2=rc, h3=reason */

    /* Sent to plugin to indicate keep alive timeout */
    PluginAction_OnLivenessCheck = 27,  /* h0=connect */

    /* Sent to plugin for a complete HTTP record */
    PluginAction_OnHttpData      = 28,  /* h0=connect, h1=op, h2=path, h3=query, p=headers+cookies, b=data */

    /* Sent to plugin to suspend message delivery */
    PluginAction_SuspendDelivery = 29,  /* h0=connect */

    /* Sent to plugin with a retained message if any to delivery. */
    PluginAction_OnGetMessage = 30, /* h0=connect */

    /* Sent to server when a new virtual connection is desired */
    PluginAction_NewConnection   = 41,   /* h0=seqnum, h1=prococol */

    /* Sent to server with stats */
    PluginAction_Stats           = 44,   /* h0=type, h1=heapsize, h2=headused, h3=gc_rate, h4=cpu */

    /* Sent to server as a synchronous reply */
    PluginAction_Reply           = 45,   /* h0=rc */

    /* Sent to server a log message */
    PluginAction_Log             = 46,   /* h0=msgid, h2=sev, h2=category, h3=file, h4=line, h5=msg, p=repl */

    /* Sent to server no reply */
    PluginAction_SendData        = 50,   /* h0=connect, b=data */

    /* Sent to server as a reply from OnConnection */
    PluginAction_Accept          = 51,   /* h0=connect, h1=protocol, h2=protocol_family, h3=plugin_name */

    /* Sent to server to request authentication, expects OnComplee */
    PluginAction_Identify        = 52,   /* h0=connect, h1=seqnum, h2=auth, h3=keepalive, h4=maxMsgInFlight, p=connection */

    /* Sent to server with a subscription request, expects OnComplete */
    PluginAction_Subscribe       = 53,   /* h0=connect, h1=seqnum, h2=flags, h3=share, h4=tansacted, h5=dest, h6=name, h7=selector */

    /* Sent to sever with an close subscription, expects OnCompletion */
    PluginAction_CloseSub        = 55,   /* h0=connect, h1=seqnum, h2=subname, h3=share */

    /* Sent to sever with an destroy subscription, expects OnCompletion */
    PluginAction_DestroySub      = 56,   /* h0=connect, h1=seqnum, h2=subname, h3=share */

    /* Sent to server with a message to send, expects OnCompletion */
    PluginAction_Send            = 57,   /* h1=oonnect, h1=seqnum, h2=msgtype, h3=flags, h4=dest, p=props, b=body  */

    /* Sent to server with a request to close a connection, expects OnComplete */
    PluginAction_Close           = 58,   /* h0=connect, h1=seqnum, h2=rc, h3=reason */

    /* Sent to server as a reply from async requests with seqnum != 0 */
    PluginAction_AsyncReply      = 59,   /* h0=connect, h1=seqnum, h2=rc, h3=id, h4=string, p=props */

    /* Sent to server to change keep alive timeout for the connection */
    PluginAction_SetKeepalive    = 60,   /* h0=connect, h1=keepAlive */

    /* Sent to server to acknowledge message delivery */
    PluginAction_Acknowledge     = 61,   /* h0=connect, h1=seqnum, h2=rc, h3=transaction */

    /* Sent to server to delete retained message */
    PluginAction_DeleteRetain    = 62,   /* h0=connect h1=topic */

    /* Sent to server to send HTTP data */
    PluginAction_SendHttp        = 63,    /* h0=connect h1=rc h2=content_type, p=map, b-body */

    /* Sent to server and to plugin to resume message delivery */
    PluginAction_ResumeDelivery        = 64,    /* h0=connect h1=msgCount */

    PluginAction_GetMessage	       	   = 65,    /* h0=connect h1=seqnum h2=topic*/

    PluginAction_CreateTransaction       = 66,  /* h0=connect h1=seqnum */

    PluginAction_CommitTransaction       = 67,  /* h0=connect h1=seqnum h2=handle */

    PluginAction_RollbackTransaction       = 68,  /* h0=connect h1=seqnum h2=handle */

    PluginAction_UpdateProperties       = 69
};

typedef struct ism_plugin_t {
    struct ism_plugin_t * next;
    const char * name;
    const char * protocol;
    const char * class;
    const char * method;
    const char * author;
    const char * version;
    const char * copyright;
    const char * build;
    const char * description;
    const char * license;
    const char * title;
    const char * alias;
    int 		 modification;
    int          classpath_count;
    int          websocket_count;
    int          httpheader_count;
    const char * classpath [32];
    const char * websocket [8];
    const char * httpheader[16];
    ism_prop_t * props;
    uint64_t     protomask;
    uint8_t      usequeue;
    uint8_t      usebrowse;
    uint8_t      usetopic;
    uint8_t      useshared;
    uint8_t      more_capabilities[2];
    uint8_t      simple_consumer;
    uint8_t      www_auth;
    uint8_t      more_flags[3];
    pthread_spinlock_t  lock;
    int          initial_byte_count;
    uint8_t      initial_byte[256];
} ism_plugin_t;

typedef struct ism_plugin_job_t ism_plugin_job_t;
/*
 * Callback for plug-in job completion
 */
typedef int (* ism_plugin_jobcall_t)(ism_plugin_job_t * job, int rc, char * buffer, int buflen);

/*
 * Asynchronous job
 */
struct ism_plugin_job_t {
    ism_plugin_job_t * prev;
    ism_plugin_job_t * next;
    ism_transport_t * channel;
    ism_transport_t * transport;
    ism_plugin_jobcall_t callback;
    int            action;
    int            which;
    int            rc;
    uint32_t       seqnum;
    ismEngine_DeliveryHandle_t deliveryh;
};

/*
 * Plugin consumer
 */
typedef struct ism_plugin_cons_t {
    void *         chandle;
    ism_transport_t * transport;
    char *   	   name;
    char *   	   dest;
    uint8_t        desttype;
    uint8_t        share;
    uint8_t        qos;
    uint8_t        closed;
    int            which;
    uint8_t		   suspended;
    uint8_t		   resrv[7];
} ism_plugin_cons_t ;

/*
 * The Plugin protocol specific area of the transport object
 */
typedef struct ism_protobj_t {
    void *         			client_handle;
    void *         			session_handle;
    const char *  			plugin_name;
    pthread_spinlock_t 		lock;
    pthread_spinlock_t 		sessionlock;
    int32_t   				inprogress;             /* Count of actions in progress */
    volatile int       		closed;                 /* Connection is not is use */
    uint32_t       			seqnum;
    uint32_t       			consumer_used;
    uint32_t       			consumer_alloc;
    int32_t        			keepAlive;
    int32_t		   			maxMsgInFlight;
    volatile int32_t 		msgInFlightCounter;
    int32_t                	state;
    uint8_t                 isGenerated;
    uint8_t                	rsrv[3];
    ism_plugin_cons_t * *   consumers;
    ism_plugin_job_t *      jobsHead;
    ism_plugin_job_t *      jobsTail;
    ismHashMap   * errors;            /* Errors that were reported already */
    ism_transport_t *       transport;
    struct ism_protobj_t *  next;
    struct ism_protobj_t *  prev;
} ism_protobj_t;

typedef ism_protobj_t	ismPluginPobj_t;

typedef struct ism_plugin_endp_t ism_plugin_endp_t;

/*
 * Endpoint modification structure
 */
struct ism_plugin_endp_t {
    ism_plugin_endp_t *  next;
    const char *   name;
    char *         buffer;     /* Properties as concise map */
    int            buflen;
};



#endif
