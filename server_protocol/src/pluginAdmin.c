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
#ifndef TRACE_COMP
#define TRACE_COMP Plugin
#endif
#include <ismutil.h>
#include <transport.h>
#define ACTION_NAMES
#define EXT_RC_STRINGS
#include "plugin.h"
#include <config.h>
#include <security.h>
#include <admin.h>
#include "imacontent.h"
#include <assert.h>
#include <pthread.h>
#include <dirent.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

int plugin_unit_test = 0;
static const char * traceFolder = IMA_SVR_DATA_PATH "/diag/logs";
const char * ism_plugin_getActionName(int action);
int ism_plugin_receive(ism_transport_t * transport, char * buf, int buflen, int command);
int ism_plugin_framechecker(ism_transport_t * transport, char * buffer, int buflen, int * used);
int ism_plugin_connection(ism_transport_t * transport);
int ism_plugin_TimerDisconnect(ism_timer_t key, ism_time_t timestamp, void * userdata);
static int plugin_startMessaging(void);
void ism_plugin_closeAllClients(int shutdown);
void ism_plugin_virtInit(void);
static volatile int pluginTerminated = 1;

/*
 * Static global variables
 */
static ism_timer_t keepAliveTimer;
static ism_timer_t requestStatsTimer;
static char taskSet[8192]={0};

#define     CHANNEL_CONNECTION_CLOSED       0
#define     CHANNEL_CONNECTION_IN_PROCESS   1
#define     CHANNEL_CONNECTED               2
#define     CHANNEL_CONNECTION_CLOSING      3

typedef struct pluginChannel_t {
    ism_transport_t *   transport;
    char *              pluginServerAddress;
    uint16_t            pluginServerPort;
    uint8_t             state;
    uint8_t             useCount;
    pthread_spinlock_t  lock;
}pluginChannel_t;

static uint32_t piSeqNum = 0;

typedef struct pluginProcessInfo_t {
    char *              vmArgs;
    char *              pluginServerIP;
    char *              debugIP;
    uint16_t            pluginPort;
    uint16_t            debugPort;
    uint16_t            maxHeap;
    volatile uint8_t    terminated;
    uint8_t             isLocal;
    pthread_barrier_t   barrier;
    pthread_mutex_t     lock;
    pthread_t           thread;
    pid_t               pid;
    uint32_t            seqNum;
    ism_timer_t         timer;
} pluginProcessInfo_t;

static pluginProcessInfo_t * currentPluginProcInfo = NULL;
static volatile int iopCount = 0;
static pluginChannel_t * channels;
static pluginChannel_t controlChannel = {0};
//static int pluginPort = 9103;
static ism_plugin_t * plugins = NULL;
static volatile int messagingStarted = 0;

static ism_plugin_endp_t * endmod_list = NULL;
static ism_plugin_job_t g_job = {0};
/*
 * Dummy endpoint so we do not segfault
 */
static ism_endstat_t    pluginstat;
static ism_endpoint_t nullEndpoint = {
    .name        = "!Plugin",
    .ipaddr      = "",
    .transport_type = "TCP",
    .protomask   = PMASK_AnyInternal,
    .stats       = &pluginstat,
    .maxSendBufferSize = 8*1024*1024,
    .maxRecvBufferSize = 8*1024*1024,
};

extern pthread_spinlock_t pluginClientsListLock;
static void * pluginProcessorMonitor(void * parm, void * context, int value);
XAPI ism_transport_t * ism_plugin_getControlChannelTransport(void);
XAPI void ism_plugin_freeControlChannelTransport(void);

static pluginProcessInfo_t * initPluginProcInfo(void) {
    ism_config_t * conhandle = ism_config_getHandle(ISM_CONFIG_COMP_PROTOCOL, NULL);
    /* Get the server dynamic config */
    ism_prop_t *  props = ism_config_getProperties(conhandle, NULL, NULL);
    pluginProcessInfo_t * result = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,16),1,sizeof(pluginProcessInfo_t));
    const char * pluginServer = ism_common_getStringProperty(props, "PluginServer");
    const char * pluginDebugServer = ism_common_getStringProperty(props, "PluginDebugServer");
    const char * pluginVMArgs = ism_common_getStringProperty(props, "PluginVMArgs");

    if((pluginServer == NULL) || (pluginServer[0] == '\0')) {
        result->pluginServerIP = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),"127.0.0.1");
    } else {
        result->pluginServerIP = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),pluginServer);
    }
    result->isLocal = !(strcmp("127.0.0.1", result->pluginServerIP));
    if(pluginDebugServer)
        result->debugIP = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),pluginDebugServer);
    if(pluginVMArgs)
        result->vmArgs = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),pluginVMArgs);

    pthread_barrier_init(&result->barrier, NULL, 2);
    pthread_mutex_init(&result->lock, NULL);
    result->pluginPort = ism_common_getIntProperty(props, "PluginPort", 9103);
    result->debugPort = ism_common_getIntProperty(props, "PluginDebugPort", 0);
    result->maxHeap = ism_common_getIntProperty(props, "PluginMaxHeapSize", 512);
    result->seqNum = piSeqNum++;
    return result;
}

static void destroyPluginProcInfo(pluginProcessInfo_t * info) {
    if(info->debugIP)
        ism_common_free(ism_memory_protocol_misc,info->debugIP);
    if(info->vmArgs)
        ism_common_free(ism_memory_protocol_misc,info->vmArgs);
    pthread_mutex_destroy(&info->lock);
    pthread_barrier_destroy(&info->barrier);
    if(info->timer)
        ism_common_cancelTimer(info->timer);
    ism_common_free(ism_memory_protocol_misc,info);
}


/*
 * Send any desired global config items from the server to the plugin process.
 */
static void makeGlobalMap(concat_alloc_t * map) {
    int sizepos = map->used;
    int len;
    ism_field_t f;

    /* Ensure we have some buffer */
    ism_protocol_ensureBuffer(map, 16);

    /*
     * Reserve space for data size
     */
    map->buf[map->used] = (char)(S_Map+3);
    map->used += 4;
    const char * trcmax = ism_common_getStringConfig("TraceMax");
    if (trcmax) {
        int trcsize = ism_common_getBuffSize("TraceMax", trcmax, "20M");
        ism_protocol_putNameValue(map, "TraceMax");
        f.type = VT_Integer;
        f.val.i = trcsize / (1024 * 1024);
        ism_protocol_putObjectValue(map, &f);
    }

    /*Set TraceLevel*/
    f.type = VT_Integer;
    f.val.i = TRACE_DOMAIN->trcLevel;
    ism_protocol_putNameValue(map, "TraceLevel");
    ism_protocol_putObjectValue(map, &f);

    f.type = VT_String;
    f.val.s = (char *)ism_common_getServerUID();
    ism_protocol_putNameValue(map, "ServerUID");
    ism_protocol_putObjectValue(map, &f);
    f.val.s = (char *)ism_common_getServerName();
    if (f.val.s) {
        ism_protocol_putNameValue(map, "ServerName");
        ism_protocol_putObjectValue(map, &f);
    }


    /*Set Plugin TraceLevel*/
    f.type = VT_Integer;
	f.val.i = TRACE_DOMAIN->trcComponentLevels[TRACECOMP_XSTR(TRACE_COMP)];
	ism_protocol_putNameValue(map, "TracePluginLevel");
	ism_protocol_putObjectValue(map, &f);


    if (ism_common_getProperty(ism_common_getConfigProperties(), "TracePluginFile", &f) == 0) {
        ism_protocol_putNameValue(map, "TracePluginFile");
        ism_protocol_putObjectValue(map, &f);
    }
    if (ism_common_getProperty(ism_common_getConfigProperties(), "TraceMessageData", &f) == 0) {
        ism_protocol_putNameValue(map, "TraceMessageData");
        ism_protocol_putObjectValue(map, &f);
    }
    if (ism_common_getProperty(ism_common_getConfigProperties(), "ConfigDir", &f) ==0 ) {
        ism_protocol_putNameValue(map, "ConfigDir");
        ism_protocol_putObjectValue(map, &f);
    }

    len = map->used - sizepos - 4;
    map->buf[sizepos+1] = (char)(len >> 16);
    map->buf[sizepos+2] = (char)((len >> 8) & 0xff);
    map->buf[sizepos+3] = (char)(len & 0xff);
}


/*
 * Include configuration functions from a separate file
 */
#include "plugin_config.c"



/*
 * Request stats from the plugin process
 */
int ism_plugin_requestStats(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    char xbuf[128];
    concat_alloc_t buf = {xbuf, sizeof xbuf, 6};
    ism_transport_t * transport = ism_plugin_getControlChannelTransport();
    if(transport) {
        ism_protocol_putIntValue(&buf, 0);
        controlChannel.transport->send(controlChannel.transport, buf.buf+6, buf.used-6, (PluginAction_GetStats<<8)+1, SFLAG_FRAMESPACE);
        ism_plugin_freeControlChannelTransport();
    }
    return 1;
}

/*
 * Convert plug-in object to a Map object
 */
static void makePluginMap(concat_alloc_t * map, ism_plugin_t * plugin, int propertiesOnly) {
    int i;
    int sizepos = map->used;
    int len;
    int capability = 0;
    char xbuf[64];

    /* Ensure we have some buffer */
    ism_protocol_ensureBuffer(map, 16);

    /*
     * Reserve space for data size
     */
    map->buf[map->used] = (char)(S_Map+3);
    map->used += 4;
    ism_protocol_putNameValue(map, "Name");
    ism_protocol_putStringValue(map, plugin->name);

    if (!propertiesOnly) {
		ism_protocol_putNameValue(map, "Protocol");
		ism_protocol_putStringValue(map, plugin->protocol);

		ism_protocol_putNameValue(map, "Class");
		ism_protocol_putStringValue(map, plugin->class);

		ism_protocol_putNameValue(map, "Author");
		ism_protocol_putStringValue(map, plugin->author);

		ism_protocol_putNameValue(map, "Version");
		ism_protocol_putStringValue(map, plugin->version);

		ism_protocol_putNameValue(map, "Copyright");
		ism_protocol_putStringValue(map, plugin->copyright);

		ism_protocol_putNameValue(map, "Build");
		ism_protocol_putStringValue(map, plugin->build);

		ism_protocol_putNameValue(map, "Description");
		ism_protocol_putStringValue(map, plugin->description);

		ism_protocol_putNameValue(map, "License");
		ism_protocol_putStringValue(map, plugin->license);

		ism_protocol_putNameValue(map, "Title");
		ism_protocol_putStringValue(map, plugin->title);

		ism_protocol_putNameValue(map, "Modification");
		ism_protocol_putIntValue(map, plugin->modification);

		ism_protocol_putNameValue(map, "Alias");
		ism_protocol_putStringValue(map, plugin->alias);

		for (i=0; i<plugin->classpath_count; i++) {
			sprintf(xbuf, "Classpath.%d", i);
			ism_protocol_putNameValue(map, xbuf);
			ism_protocol_putStringValue(map, plugin->classpath[i]);
		}

		for (i=0; i<plugin->websocket_count; i++) {
			sprintf(xbuf, "WebSocket.%d", i);
			ism_protocol_putNameValue(map, xbuf);
			ism_protocol_putStringValue(map, plugin->websocket[i]);
		}
		for (i=0; i<plugin->httpheader_count; i++) {
		   sprintf(xbuf, "HttpHeader.%d", i);
		   ism_protocol_putNameValue(map, xbuf);
		   ism_protocol_putStringValue(map, plugin->httpheader[i]);
		}

		ism_protocol_putNameValue(map, "InitialByte");
		if (plugin->initial_byte_count == 256) {
			ism_protocol_putStringValue(map, "All");
		} else if (plugin->initial_byte_count == 0) {
			ism_protocol_putStringValue(map, "None");
		} else {
			int count = 0;
			ism_protocol_putStringValue(map, "List");
			for (i=0; i<256; i++) {
				if (plugin->initial_byte[i]) {
					sprintf(xbuf, "InitialByte.%d", count++);
					ism_protocol_putNameValue(map, xbuf);
					ism_protocol_putIntValue(map, i);
				}
			}
		}

		if (plugin->usetopic)
			capability |= ISM_PROTO_CAPABILITY_USETOPIC;
		if (plugin->useshared)
			capability |= ISM_PROTO_CAPABILITY_USESHARED;
		if (plugin->usequeue)
			capability |= ISM_PROTO_CAPABILITY_USEQUEUE;
		if (plugin->usebrowse)
			capability |= ISM_PROTO_CAPABILITY_USEBROWSE;
		ism_protocol_putNameValue(map, "Capabilities");
		ism_protocol_putIntValue(map, capability);

		ism_protocol_putNameValue(map, "ProtocolMask");
		ism_protocol_putLongValue(map, plugin->protomask);
    }

    ism_protocol_putNameValue(map, "Properties");
    pthread_spin_lock(&plugin->lock);
    if (plugin->props)
        ism_protocol_putMapProperties(map, plugin->props);
    else
        ism_protocol_putNullValue(map);
    pthread_spin_unlock(&plugin->lock);
    len = map->used - sizepos - 4;
    map->buf[sizepos+1] = (char)(len >> 16);
    map->buf[sizepos+2] = (char)((len >> 8) & 0xff);
    map->buf[sizepos+3] = (char)(len & 0xff);
}

static inline ism_transport_t * getControlChannelTransport(void) {
    ism_transport_t * transport = NULL;
    pthread_spin_lock(&controlChannel.lock);
    if(LIKELY(controlChannel.transport && (controlChannel.state == CHANNEL_CONNECTED))) {
        transport = controlChannel.transport;
        controlChannel.useCount++;
    }
    pthread_spin_unlock(&controlChannel.lock);
    return transport;
}


static inline ism_transport_t * getChannelTransport(int which) {
    ism_transport_t * transport = NULL;
    pthread_spin_lock(&channels[which].lock);
    if (LIKELY(channels[which].transport && (channels[which].state == CHANNEL_CONNECTED))) {
        transport = channels[which].transport;
        channels[which].useCount++;
    }
    pthread_spin_unlock(&channels[which].lock);
    return transport;
}
/*
 *
 */
ism_transport_t * ism_plugin_getChannelTransport(int which) {
    ism_transport_t * transport = NULL;
    if(!pluginTerminated) {
        transport = getChannelTransport(which);
    }
    return transport;
}

/*
 *
 */
static inline void freeChannelTransport(int which) {
    ism_transport_t * transport = NULL;
    char * server = NULL;
    pthread_spin_lock(&channels[which].lock);
    channels[which].useCount--;
    if(channels[which].useCount == 0) {
        transport = channels[which].transport;
        server = channels[which].pluginServerAddress;
        channels[which].transport = NULL;
        channels[which].pluginServerAddress = NULL;
        channels[which].state = CHANNEL_CONNECTION_CLOSED;
    }
    pthread_spin_unlock(&channels[which].lock);
    if(UNLIKELY(transport != NULL)) {
    	TRACE(4, "ism_plugin_freeChannelTransport: complete transport closing for channel %u transport=%p connection=%u\n",
    			transport->clientport, transport, transport->index);
        transport->closed(transport);
    }
    if(UNLIKELY(server != NULL)) {
        ism_common_free(ism_memory_protocol_misc,server);
    }
}

XAPI void ism_plugin_freeChannelTransport(int which) {
    freeChannelTransport(which);
}

/*
 * Convert endpoint object to a Map object
 */
static void makeEndpointMap(concat_alloc_t * map, ism_endpoint_t * endp) {
    int sizepos = map->used;
    int len;

    /* Ensure we have some buffer */
    ism_protocol_ensureBuffer(map, 16);

    /*
     * Reserve space for data size
     */
    map->buf[map->used] = (char)(S_Map+3);
    map->used += 4;
    ism_protocol_putNameValue(map, "Name");
    ism_protocol_putStringValue(map, endp->name);

    ism_protocol_putNameValue(map, "Enabled");
    ism_protocol_putIntValue(map, endp->enabled);

    ism_protocol_putNameValue(map, "ErrorCode");
    ism_protocol_putIntValue(map, endp->rc);

    ism_protocol_putNameValue(map, "Interface");
    ism_protocol_putStringValue(map, endp->ipaddr);

    ism_protocol_putNameValue(map, "Port");
    ism_protocol_putIntValue(map, endp->port);

    ism_protocol_putNameValue(map, "MessageHub");
    ism_protocol_putStringValue(map, endp->msghub);

    ism_protocol_putNameValue(map, "Transport");
    ism_protocol_putStringValue(map, endp->transport_type);

    ism_protocol_putNameValue(map, "Secure");
    ism_protocol_putIntValue(map, endp->secure);

    ism_protocol_putNameValue(map, "MaxMessageSize");
    ism_protocol_putIntValue(map, endp->maxMsgSize);

    ism_protocol_putNameValue(map, "UseClientCertificate");
    ism_protocol_putIntValue(map, endp->useClientCert);

    ism_protocol_putNameValue(map, "UsePassword");
    ism_protocol_putIntValue(map, endp->usePasswordAuth);

    ism_protocol_putNameValue(map, "ProtocolMask");
    ism_protocol_putLongValue(map, endp->protomask);


    len = map->used - sizepos - 4;
    map->buf[sizepos+1] = (char)(len >> 16);
    map->buf[sizepos+2] = (char)((len >> 8) & 0xff);
    map->buf[sizepos+3] = (char)(len & 0xff);
}

/*
 *
 */
static int pluginInitChannel(ism_transport_t * transport) {
    transport->actionname = ism_plugin_getActionName;
    transport->protocol = "plugin"; /* Make the string constant */
    transport->protocol_family = "plugin"; /* Constant string */
    transport->endpoint_name = "plugin";
    transport->client_addr="";
    transport->server_addr="";
    transport->originated = 2;      /* Allow us to set clientport and tid */
    transport->receive = ism_plugin_receive;
    return 0;
}

/*
 *
 */
static int messagingConnectionComplete(ism_transport_t * transport, int rc) {
	int isOpen = 0;
	pluginChannel_t * channel = &channels[transport->clientport];

    pthread_spin_lock(&channel->lock);
    if ((rc == 0) && (channel->state == CHANNEL_CONNECTION_IN_PROCESS)) {
    	channel->state = CHANNEL_CONNECTED;
    	transport->ready = 1;
        isOpen = 1;
    } else {
    	transport = channel->transport;
    	channel->transport = NULL;
    	channel->state = CHANNEL_CONNECTION_CLOSED;
    	channel->useCount = 0;
    }
    pthread_spin_unlock(&channel->lock);

    if (isOpen) {
        char xbuf[128];
        transport->ready = 1;
        concat_alloc_t buf = {xbuf, sizeof xbuf, 6};
        TRACE(4, "Plugin channel %u connected: transport=%p connection=%u\n", transport->clientport, transport, transport->index);
        ism_protocol_putIntValue(&buf, transport->clientport);
        transport->send(transport, buf.buf+6, buf.used-6, (PluginAction_InitChannel<<8)+1, SFLAG_FRAMESPACE);
    } else {
    	if (transport)
    		transport->closed(transport);
    }
    return 0;
}

/*
 *
 */
int messagingChannelClosing(ism_transport_t * transport, int rc, int clean, const char * reason) {
    pluginChannel_t * channel = &channels[transport->clientport];
    pthread_spin_lock(&channel->lock);
    if ((channel->state == CHANNEL_CONNECTION_CLOSED) ||
        (channel->state == CHANNEL_CONNECTION_CLOSING)) {
    	pthread_spin_unlock(&channel->lock);
    	return 0;
    }
    if (channel->state == CHANNEL_CONNECTION_IN_PROCESS) {
    	channel->state = CHANNEL_CONNECTION_CLOSED;
        channel->transport = NULL;
        channel->useCount = 0;
        if(channel->pluginServerAddress) {
            ism_common_free(ism_memory_protocol_misc,channel->pluginServerAddress);
            channel->pluginServerAddress = NULL;
        }
    	pthread_spin_unlock(&channel->lock);
   		transport->closed(transport);
    	return 0;
    }
    channel->useCount--;
    if (channel->useCount) {
        channel->state = CHANNEL_CONNECTION_CLOSING;
        pthread_spin_unlock(&channel->lock);
        return 0;
    }
    channel->state = CHANNEL_CONNECTION_CLOSED;
    channel->transport = NULL;
    if(channel->pluginServerAddress) {
        ism_common_free(ism_memory_protocol_misc,channel->pluginServerAddress);
        channel->pluginServerAddress = NULL;
    }
    pthread_spin_unlock(&channel->lock);
	TRACE(4, "messagingChannelClosing: complete transport closing for channel %u transport=%p connection=%u\n",
			transport->clientport, transport, transport->index);
    transport->closed(transport);
	return 0;
}

/*
 *
 */
static int startMessagingChannel(int which) {
    if (pluginTerminated)
        return 0;
    if(ism_plugin_getControlChannelTransport()) {
        char name[32];
        ism_transport_t * transport = ism_transport_newOutgoing(&nullEndpoint, 1);
        pluginInitChannel(transport);
        pthread_spin_lock(&channels[which].lock);
        channels[which].transport = transport;
        channels[which].state = CHANNEL_CONNECTION_IN_PROCESS;
        channels[which].pluginServerAddress = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),controlChannel.pluginServerAddress);
        channels[which].pluginServerPort = controlChannel.pluginServerPort;
        channels[which].useCount = 1;
        pthread_spin_unlock(&channels[which].lock);
        ism_plugin_freeControlChannelTransport();
        transport->clientport = which;
        transport->serverport = channels[which].pluginServerPort;
        transport->tid = which % iopCount;
        sprintf(name,"pluginChannel.%d",which);
        char * tmp = ism_transport_allocBytes(transport, strlen(name)+1, 1);
        strcpy(tmp,name);
        transport->name = tmp;
        transport->closing = messagingChannelClosing;
        transport->connected = messagingConnectionComplete;
        /* Start the control connection to the plug-in process */
        TRACE(5, "Start outgoing messaging channel connection with plug-in process: index=%d\n",which);
        ism_transport_connect(transport, transport, channels[which].pluginServerAddress, channels[which].pluginServerPort, NULL);
    }
    return 0;
}

static int startControlChannel(uint32_t seqNum);

/*
 *
 */
static int pluginStartControlChannelTimer(ism_timer_t timer, ism_time_t timestamp, void * userdata) {
    uint32_t seqNum = (uint32_t) ((uintptr_t) userdata);
    startControlChannel(seqNum);
    ism_common_cancelTimer(timer);
    return 0;
}


/*
 *
 */
static void handleControlChannelClose(ism_time_t delay) {
    pthread_spin_lock(&controlChannel.lock);
    if(currentPluginProcInfo)
        currentPluginProcInfo->timer =
                ism_common_setTimerOnce(ISM_TIMER_HIGH, pluginStartControlChannelTimer, (void*) ((uintptr_t)currentPluginProcInfo->seqNum), delay);  /* 1 second */
    pthread_spin_unlock(&controlChannel.lock);
}


/*
 *
 */
static int controlConnectionComplete(ism_transport_t * transport, int rc) {
	int isOpen = 0;
    pthread_spin_lock(&controlChannel.lock);
    if ((rc == 0) && (controlChannel.state == CHANNEL_CONNECTION_IN_PROCESS)) {
    	controlChannel.state = CHANNEL_CONNECTED;
    	transport->ready = 1;
        isOpen = 1;
    } else {
    	transport = controlChannel.transport;
    	controlChannel.transport = NULL;
    	controlChannel.state = CHANNEL_CONNECTION_CLOSED;
        controlChannel.useCount = 0;
    }
    pthread_spin_unlock(&controlChannel.lock);
    if (isOpen) {
    	if (plugins) {
            char xbuf[128];
            concat_alloc_t buf = {xbuf, sizeof xbuf, 6};
            TRACE(4, "Plugin control channel connected: transport=%p connection=%u\n", transport, transport->index);
            ism_protocol_putIntValue(&buf, transport->clientport);
            g_job.action = PluginAction_Initialize;
            makeGlobalMap(&buf);
            transport->send(transport, buf.buf+6, buf.used-6, (PluginAction_Initialize<<8)+1, SFLAG_FRAMESPACE);
    	}
    } else {
    	if (transport) {
			transport->closed(transport);
			handleControlChannelClose(30000000000);
    	}
    }
    return 0;
}

static void completeControlChannelClose(void) {
int i;
ism_transport_t * transport = NULL;
    TRACE(4, "completeControlChannelClose: complete control channel closing\n");
    for(i = 0; i < iopCount; i++) {
        ism_transport_t * channel = ism_plugin_getChannelTransport(i);
        if(channel) {
            channel->close(channel, ISMRC_OK, 0,"Control channel was closed ");
            ism_plugin_freeChannelTransport(i);
        }
    }

    ism_plugin_closeAllClients(pluginTerminated);
    pthread_spin_lock(&controlChannel.lock);
    __sync_fetch_and_and(&messagingStarted,1);
    controlChannel.state = CHANNEL_CONNECTION_CLOSED;
    transport = controlChannel.transport;
    controlChannel.transport = NULL;
    if(controlChannel.pluginServerAddress) {
        ism_common_free(ism_memory_protocol_misc,controlChannel.pluginServerAddress);
        controlChannel.pluginServerAddress = NULL;
    }
    pthread_spin_unlock(&controlChannel.lock);
    if(transport)
        transport->closed(transport);
    handleControlChannelClose(30000000000);
}

/*
 * TODO:
 */
static int controlChannelClosing(ism_transport_t * transport, int rc, int clean, const char * reason) {
    __sync_fetch_and_and(&messagingStarted,0x1);
    pthread_spin_lock(&controlChannel.lock);
    if ((controlChannel.state == CHANNEL_CONNECTION_CLOSED) ||
        (controlChannel.state == CHANNEL_CONNECTION_CLOSING)) {
    	pthread_spin_unlock(&controlChannel.lock);
    	return 0;
    }
    if (controlChannel.state == CHANNEL_CONNECTION_IN_PROCESS) {
    	controlChannel.state = CHANNEL_CONNECTION_CLOSED;
        controlChannel.transport = NULL;
        if(controlChannel.pluginServerAddress) {
            ism_common_free(ism_memory_protocol_misc,controlChannel.pluginServerAddress);
            controlChannel.pluginServerAddress = NULL;
        }
        controlChannel.useCount = 0;
    	pthread_spin_unlock(&controlChannel.lock);
   		transport->closed(transport);
    	return 0;
    }
    controlChannel.useCount--;
    if (controlChannel.useCount) {
        controlChannel.state = CHANNEL_CONNECTION_CLOSING;
        pthread_spin_unlock(&controlChannel.lock);
        return 0;
    }
    pthread_spin_unlock(&controlChannel.lock);
    completeControlChannelClose();
	return 0;
}

/*
 * Start a connection to the plug-in.  This completes in ism_plugin_connected.
 */
static int startControlChannel(uint32_t seqNum) {
	if (pluginTerminated)
		return 0;
    ism_transport_t * transport = ism_transport_newOutgoing(&nullEndpoint, 1);
    pluginInitChannel(transport);
    pthread_spin_lock(&controlChannel.lock);
    if((currentPluginProcInfo == NULL) || (currentPluginProcInfo->seqNum != seqNum) ||
        (currentPluginProcInfo->timer == NULL)) {
        ism_transport_freeTransport(transport);
        pthread_spin_unlock(&controlChannel.lock);
        return 0;
    }
    currentPluginProcInfo->timer = NULL;
    controlChannel.transport = transport;
    controlChannel.state = CHANNEL_CONNECTION_IN_PROCESS;
    controlChannel.pluginServerAddress = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),currentPluginProcInfo->pluginServerIP);
    controlChannel.pluginServerPort = currentPluginProcInfo->pluginPort;
    controlChannel.useCount = 1;
    pthread_spin_unlock(&controlChannel.lock);
    transport->name = "pluginControl";
    transport->clientport = 0xFFFF;
    transport->serverport = controlChannel.pluginServerPort;
    transport->tid = 0;
    transport->connected = controlConnectionComplete;
    transport->closing = controlChannelClosing;
    /* Start the control connection to the plug-in process */
    TRACE(5, "Start outgoing control connection with plug-in process\n");
    ism_transport_connect(transport, transport, controlChannel.pluginServerAddress, controlChannel.pluginServerPort, NULL);

    return 0;
}

/*
 * Initialize the plug-in protocol
 */
int ism_protocol_initPlugin(void) {
    int i, affLen;
    char affMap[CPU_SETSIZE] = {0};

    const char * tf = ism_common_getStringConfig("TraceFolder");
    if(tf)
        traceFolder = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),tf);
    plugin_unit_test = (getenv("CUNIT") != NULL);
    TRACE(5, "Initialize plug-in protocol\n");
    ism_transport_registerFramer(ism_plugin_framechecker);
    ism_transport_registerProtocol(plugin_startMessaging, ism_plugin_connection);
    keepAliveTimer = ism_common_setTimerRate(ISM_TIMER_LOW, ism_plugin_TimerDisconnect, NULL, 1, 30, TS_SECONDS);
    requestStatsTimer = ism_common_setTimerRate(ISM_TIMER_LOW, ism_plugin_requestStats, NULL, 20, 30, TS_SECONDS);
    pthread_spin_init(&pluginClientsListLock, 0);
    pthread_spin_init(&controlChannel.lock, 0);
    iopCount = ism_tcp_getIOPcount();
    TRACE(4, "Start plug-in: iopCount=%d\n", iopCount);
    nullEndpoint.thread_count = iopCount;
    channels = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,25),iopCount,sizeof(pluginChannel_t));
    for(i = 0; i < iopCount; i++) {
        pthread_spin_init(&channels[i].lock, 0);
    }
    affLen = ism_config_autotune_getaffinity("Plugin",affMap);
    if (affLen){
        int j;
        for(j = 0; j < affLen; j++) {
            if(affMap[j]){
                char cpu[16];
                sprintf(cpu,"%d,",j);
                strcat(taskSet,cpu);
            }
        }
        taskSet[strlen(taskSet)-1] = '\0';
    }

    if (iopCount > MAX_STAT_THREADS) {
            /* Increase the stats size if we have a lot of IOP threads */
        nullEndpoint.stats = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,26),1, sizeof(ism_endstat_t) + ((iopCount-MAX_STAT_THREADS) * sizeof(msg_stat_t)));
    }
    ism_plugin_virtInit();
    return ISMRC_OK;
}

static int updatePlugins(void);
/*
 * Start the plugin
 */
int ism_protocol_startPlugin(void) {
    int rc = ISMRC_OK;
    TRACE(5, "Start plug-in protocol: pluginTerminated=%d\n", pluginTerminated);
    if (__sync_bool_compare_and_swap(&pluginTerminated, 1, 0)) {
        updatePlugins();
        rc = configPlugin();
        if (plugins) {
            pthread_spin_lock(&controlChannel.lock);
            assert(currentPluginProcInfo == NULL);
            currentPluginProcInfo = initPluginProcInfo();
            if(currentPluginProcInfo->isLocal) {
                ism_common_startThread(&currentPluginProcInfo->thread, pluginProcessorMonitor, currentPluginProcInfo,
                        NULL, 0, ISM_TUSAGE_NORMAL, 0, "PlugMon", "Plugin Process Monitor");
                pthread_barrier_wait(&currentPluginProcInfo->barrier);
            }
            currentPluginProcInfo->timer =
                    ism_common_setTimerOnce(ISM_TIMER_HIGH, pluginStartControlChannelTimer, (void*)((uintptr_t)currentPluginProcInfo->seqNum), 1000000000);  /* 1 second */
            pthread_spin_unlock(&controlChannel.lock);
        } else {
            __sync_bool_compare_and_swap(&pluginTerminated, 0, 1);
        }
    }
    return rc;
}

/*
 * This will be called when need to restart the plugin.
 * If this is the first time, need to call ism_protocol_startPlugin
 * so the objects are initialized.
 */
int ism_protocol_restartPlugin(void) {
    ism_protocol_termPlugin();
    ism_protocol_startPlugin();
    return 0;
}


ism_endpoint_t * ism_transport_findEndpoint(const char * name);

/*
 * Configure endpoints
 */
static void configEndpoints(void) {
    int  i;
    int  count;
    ism_endpoint_mon_t * endlist;
    ism_plugin_endp_t * endp;

    count = ism_transport_getEndpointMonitor(&endlist, "*");
    for (i=0; i<count; i++) {
        char xbuf [4096];
        concat_alloc_t buf = {xbuf, sizeof xbuf};
        ism_endpoint_t * endpoint = ism_transport_findEndpoint(endlist[i].name);
        if (endpoint) {
            makeEndpointMap(&buf, endpoint);
            endp = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,27),1, sizeof(ism_plugin_endp_t) + strlen(endlist[i].name) + 1 + buf.used + 1);
            endp->name = (char *)(endp+1);
            strcpy((char *)endp->name, endlist[i].name);
            endp->buffer = (char *)endp->name + strlen(endp->name) + 1;
            memcpy(endp->buffer, buf.buf, buf.used);
            endp->buflen = buf.used;
            endp->next = endmod_list;
            endmod_list = endp;
            if (buf.inheap)
                ism_common_freeAllocBuffer(&buf);
        }
    }
    ism_transport_freeEndpointMonitor(endlist);
    g_job.action = PluginAction_Endpoint;
}

/*
 * This is used only in the control channel communications.
 */
int ism_plugin_replyControl(ism_transport_t * transport, int rc) {
    ism_plugin_t * plugin;
    char xbuf [4096];
    concat_alloc_t buf = {xbuf, sizeof xbuf, 6};
    ism_plugin_endp_t * endpoint;
    int  i;

    if (rc != 0) {
        TRACE(1, "Plugin reply: channel=%d rc=%d\n", transport->clientport, rc);
        g_job.action = 0;
        return -1;
        /* TODO: error handling */
    }
    TRACE(5, "ism_plugin_replyControl: channel=%d action=%d\n", transport->clientport, g_job.action);
    if (transport->clientport != 0xFFFF) {
    	/* This is not a control channel - NOP*/
    	return 0;
    }
    if (((controlChannel.state != CHANNEL_CONNECTED) || (controlChannel.transport != transport)) && (g_job.action != PluginAction_Terminate)) {
        return 0;
    }

    switch (g_job.action) {
    case PluginAction_Initialize:
    	TRACE(4, "PluginAction_Initialize: Plugin control channel initialized\n");

        g_job.action = PluginAction_DefinePlugin;
        g_job.which = 0;
        for(i = 0; i <iopCount; i++)
        	startMessagingChannel(i);
        /* fall thru */
    case PluginAction_DefinePlugin:
        TRACE(4, "PluginAction_DefinePlugin: messagingStarted=0x%x job.which=%d\n",messagingStarted, g_job.which);
        plugin = plugins;
        for (i=0; i<g_job.which; i++) {
            if (!plugin)
                break;
            plugin = plugin->next;
        }
        g_job.which++;
        if (plugin) {
        	TRACE(4, "Define plugin: %s\n", plugin->name);
            makePluginMap(&buf, plugin,0);
            transport->send(transport, buf.buf+6, buf.used-6, (PluginAction_DefinePlugin<<8)+0, SFLAG_FRAMESPACE);
            return 0;
        }

        g_job.which = 0;
        if ((__sync_fetch_and_or(&messagingStarted,2) & 1) == 0) {
        	TRACE(4, "All plugins are defined but startMessaging was not called yet. messagingStarted=0x%x\n", messagingStarted);
        	break;
        }
    	TRACE(4, "All plugins are defined - going to config endpoints. messagingStarted=0x%x\n", messagingStarted);
        configEndpoints();
        /* fall thru */
    case PluginAction_Endpoint:
    	/* TODO: take care of synchronization of endmod_list updates */
        TRACE(4, "PluginAction_Endpoint: messagingStarted=0x%x job.which=%d\n",messagingStarted, g_job.which);
        endpoint = endmod_list;
        if (endpoint) {
            endmod_list = endpoint->next;
            char * pos = ism_common_allocAllocBuffer(&buf, endpoint->buflen, 0);
            memcpy(pos, endpoint->buffer, endpoint->buflen);
        	TRACE(4, "Configure plugin endpoint: %s\n", endpoint->name);
            transport->send(transport, buf.buf+6, buf.used-6, (PluginAction_Endpoint<<8)+0, SFLAG_FRAMESPACE);
            ism_common_free(ism_memory_protocol_misc,endpoint);
            g_job.which++;
            break;
        }

        if ((__sync_fetch_and_or(&messagingStarted,4) & 4) == 0) {
			g_job.action = PluginAction_StartMessaging;
			g_job.which = 0;
			ism_protocol_putByteValue(&buf, 0);
	    	TRACE(4, "All plugin endpoints are configured - send StartMessaging request\n");
			transport->send(transport, buf.buf+6, buf.used-6, (PluginAction_StartMessaging<<8)+1, SFLAG_FRAMESPACE);
			break;
        } else {
            TRACE(4, "All plugin endpoints are configured - Start Messaging request has been sent already\n");
        }
        /* fall thru */
    case PluginAction_StartMessaging:
    	__sync_fetch_and_or(&messagingStarted,8);
        TRACE(4, "PluginAction_StartMessaging: messagingStarted=0x%x\n",messagingStarted);
        g_job.action = 0;
        break;
    case PluginAction_Terminate:
        /* TODO: continue with terminate */
        break;
    }
    return 0;
}



/*
 * Indicate to the plug-in that messaging has started
 */
static int plugin_startMessaging(void) {
    int started = __sync_fetch_and_or(&messagingStarted,1);
    TRACE(4, "plugin_startMessaging: messagingStarted=0x%x\n", messagingStarted);
    if (plugins) {
        ism_transport_t* transport = ism_plugin_getControlChannelTransport();
        if(transport) {
            if ((started & 2) == 0) {
                TRACE(4, "plugin_startMessaging: wait for all plugins defined\n");
            } else {
                TRACE(4, "plugin_startMessaging: going to config endpoints\n");
                configEndpoints();
                ism_plugin_replyControl(transport, 0);
            }
            ism_plugin_freeControlChannelTransport();
        } else {
            TRACE(4, "plugin_startMessaging: wait for control channel connection\n");
        }
    }
	return 0;
}

/*
 * Find a plug-in by name
 */
ism_plugin_t * ism_plugin_findByName(const char * name) {
    if (!name)
        return NULL;
    ism_plugin_t * plugin = plugins;
    while (plugin) {
        if (!strcmp(name, plugin->name))
            break;
        plugin = plugin->next;
    }
    return plugin;
}

/*
 * Find by WebSockets protocol
 */
ism_plugin_t * ism_plugin_findByWSProtocol(const char * protocol) {
    if (!protocol || !*protocol)
        return NULL;
    ism_plugin_t * plugin = plugins;
    while (plugin) {
        int i;
        if (*protocol == '/') {
            if (plugin->alias && !strcmp(protocol, plugin->alias))
                break;
        } else {
            for (i=0; i<plugin->websocket_count; i++) {
                if (strcmpi(protocol, plugin->websocket[i]) == 0) {
                    return plugin;
                }
            }
        }
        plugin = plugin->next;
    }
    return plugin;
}

/*
 * Find a protocol with a matching first byte
 */
ism_plugin_t * ism_plugin_findByFirstByte(uint8_t b) {
    ism_plugin_t * plugin = plugins;
    while (plugin) {
        if ((plugin->initial_byte_count == 0) || (plugin->initial_byte[b]))
            break;
        plugin = plugin->next;
    }
    return plugin;
}

/*
 * Find a protocol with a matching the HTTP alias
 */
ism_plugin_t * ism_plugin_findByAlias(const char * protocol) {
    ism_plugin_t * plugin = plugins;
    while (plugin) {
        if (plugin->alias && !strcmp(protocol, plugin->alias))
            return plugin;
        plugin = plugin->next;
    }
    return NULL;
}

static void killPluginProcess(void);

XAPI void ism_protocol_termPlugin(void) {
    TRACE(5, "Terminate plug-in protocol: pluginTerminated=%d\n", pluginTerminated);
    if (__sync_bool_compare_and_swap(&pluginTerminated, 0, 1)) {
        if (plugins) {
            int i;
            pthread_spin_lock(&controlChannel.lock);
            if(currentPluginProcInfo)
                currentPluginProcInfo->terminated = 1;
            pthread_spin_unlock(&controlChannel.lock);

            for(i = 0; i < iopCount; i++) {
                ism_transport_t * channel = getChannelTransport(i);
                if (channel) {
                    channel->close(channel, ISMRC_OK, 0,"Plug-in protocol was terminated");
                    ism_plugin_freeChannelTransport(i);
                }
            }
            ism_transport_t * tran = getControlChannelTransport();
            if (tran) {
                tran->close(tran, ISMRC_OK, 0, "Plug-in protocol was terminated");
                ism_plugin_freeControlChannelTransport();
            }
            for(i = 0; i < 100; i++ ) {
                if(controlChannel.state == CHANNEL_CONNECTION_CLOSED)
                    break;
                ism_common_sleep(10000);
            }
            /* Empty the plugins list*/
            ism_plugin_removeAllPlugins();
            pthread_spin_lock(&controlChannel.lock);
            if(currentPluginProcInfo) {
                if(currentPluginProcInfo->isLocal) {
                    void * result = NULL;
                    pthread_mutex_lock(&currentPluginProcInfo->lock);
                    currentPluginProcInfo->terminated = 1;
                    if(currentPluginProcInfo->pid) {
                        killPluginProcess();
                    }
                    pthread_mutex_unlock(&currentPluginProcInfo->lock);
                    pthread_join(currentPluginProcInfo->thread, &result);
                }
            }
            destroyPluginProcInfo(currentPluginProcInfo);
            currentPluginProcInfo = NULL;
            pthread_spin_unlock(&controlChannel.lock);
        }
    }
}

XAPI ism_transport_t * ism_plugin_getControlChannelTransport(void) {
    ism_transport_t * transport = NULL;
    if(!pluginTerminated) {
        transport = getControlChannelTransport();
    }
    return transport;
}

static inline void freeControlChannelTransport(void) {
    int shouldClose = 0;
    pthread_spin_lock(&controlChannel.lock);
    controlChannel.useCount--;
    if(controlChannel.useCount == 0) {
        shouldClose = 1;
    }
    pthread_spin_unlock(&controlChannel.lock);
    if(shouldClose) {
    	completeControlChannelClose();
    }
}
XAPI void ism_plugin_freeControlChannelTransport(void) {
    freeControlChannelTransport();
}

static pid_t  createPluginProcess(pluginProcessInfo_t * procInfo) {
    char * argv[64] = {NULL};
    char * env[64] = {NULL};
    const char * cfg;
    int index = 1;
    pid_t pid;
    argv[0] = IMA_SVR_INSTALL_PATH "/bin/startPluginService.sh";
    cfg = ism_common_getStringConfig("ConfigDir");
    if(cfg) {
        argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),"-c");
        argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),cfg);
    }

    argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),"-l");
    argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),traceFolder);
    if(taskSet[0]) {
        argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),"-s");
        argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),taskSet);
    }

    if(procInfo->pluginPort) {
        char * data = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,29),32);
        sprintf(data,"%d",procInfo->pluginPort);
        argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),"-p");
        argv[index++] = data;
    }

    if(procInfo->debugPort) {
        char * data = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,30),256);
        if(procInfo->debugIP && procInfo->debugIP[0]) {
            sprintf(data,"%s:%d", procInfo->debugIP, procInfo->debugPort);
        } else {
            sprintf(data,"%d", procInfo->debugPort);
        }
        argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),"-d");
        argv[index++] = data;
    }

    if(procInfo->maxHeap) {
        char * data = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,31),32);
        sprintf(data,"%d",procInfo->maxHeap);
        argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),"-x");
        argv[index++] = data;
    }
    if(procInfo->vmArgs && procInfo->vmArgs[0]) {
        argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),"-v");
        argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),procInfo->vmArgs);
    }
    pid = vfork();
    if(!pid) {
        //Child process
        char logFile[512];
        sprintf(logFile, "%s/pluginStartup",traceFolder);
        int fd = open(logFile, O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
        dup2(fd,1);
        dup2(fd,2);
        close(fd);
        execve(argv[0], argv, env);
        _exit(errno);
    }
    for(--index; index > 0; --index) {
        if(argv[index])
            ism_common_free(ism_memory_protocol_misc,argv[index]);
    }
    return pid;
}

static void * pluginProcessorMonitor(void * parm, void * context, int value) {
    int shouldNotify = 1;
    pluginProcessInfo_t * procInfo = (pluginProcessInfo_t*)parm;
//    procInfo->thread = pthread_self();
    pthread_mutex_lock(&procInfo->lock);
    while(!procInfo->terminated) {
        pid_t pid = createPluginProcess(procInfo);
        if(shouldNotify) {
            ism_common_sleep(10000);
            pthread_barrier_wait(&procInfo->barrier);
            shouldNotify = 0;
        }
        if(pid > 0) {
            int status = 0;
            procInfo->pid = pid;
            pthread_mutex_unlock(&procInfo->lock);
            waitpid(pid, &status, 0);
            pthread_mutex_lock(&procInfo->lock);
            //Make sure that java process is dead too
            killPluginProcess();
            procInfo->pid = 0;
            continue;
        }
        break;
    }
    pthread_mutex_unlock(&procInfo->lock);
    return NULL;
}



static int invokeScript(const char * script, char ** argv, char ** env, int append) {
    pid_t pid;
    int err, status;
    char cmd[1024];
    char logFile[PATH_MAX];
    int flag = O_RDWR | O_CREAT;
    sprintf(cmd, IMA_SVR_INSTALL_PATH "/bin/%s.sh", script);
    if(append) {
        snprintf(logFile,sizeof(logFile),"%s/%s.log", traceFolder, script);
        flag |= O_APPEND;
    } else {
        snprintf(logFile,sizeof(logFile),"%s/%s.%llu.log", traceFolder, script, ((long long unsigned int)time(NULL)));
    }
    argv[0] = cmd;
    pid = vfork();
    if(!pid) {
        //Child process
        int fd = open(logFile, flag, S_IRUSR | S_IWUSR);
        dup2(fd,1);
        dup2(fd,2);
        close(fd);
        execve(argv[0], argv, env);
        _exit(errno);
    }
    err = errno;
    if(pid < 0) {
        ism_common_setErrorData(ISMRC_SysCallFailed, "%s%d%s", "vfork", err, strerror(err));
        unlink(logFile);
        return ISMRC_SysCallFailed;
    }
    waitpid(pid, &status, 0);
    err = WEXITSTATUS(status);
    if(WIFEXITED(status) && (err == 0)) {
        if(unlink(logFile)) {
            int e = errno;
            TRACE(4, "Could not remove log file %s. The error is %s(%d)\n", logFile, strerror(e), e);
        }
        TRACE(5, "%s invoked successfully\n", cmd);
        return ISMRC_OK;
    }
    if(err == 255) {
        if(!append){
            char * buf;
            int length;
            if(ism_common_readFile(logFile, &buf, &length) == ISMRC_OK) {
                ism_common_setErrorData(ISMRC_PluginUpdateError, "%s", buf);
                ism_common_free(ism_memory_protocol_misc,buf);
            } else {
                ism_common_setErrorData(ISMRC_PluginUpdateError, "%s", "Unknown");
            }
            if(unlink(logFile)) {
                int e = errno;
                TRACE(4, "Could not remove log file %s. The error is %s(%d)\n", logFile, strerror(e), e);
            }
        }
        return ISMRC_PluginUpdateError;
    }
    ism_common_setErrorData(ISMRC_SysCallFailed, "%s%d%s", "execve", err, strerror(err));
    return ISMRC_SysCallFailed;
}

static void killPluginProcess(void) {
    char * argv[8] = {NULL};
    char * env[8] = {NULL};
    invokeScript("stopPluginProc", argv, env, 1);
}

/*
 * Update properties
 */
static void updatePluginProperties(const char * pluginName) {
    ism_transport_t * transport=NULL;
    ism_plugin_t * plugin = ism_plugin_findByName(pluginName) ;
    if (plugin) {
        /*Read in Properties.*/
        char propsFilePath[1024];

        memset(propsFilePath, '\0', sizeof(propsFilePath));
        snprintf(propsFilePath, sizeof(propsFilePath), "%s%s/pluginproperties.json", STAGING_INSTALL_DIR, plugin->name);
        ism_plugin_process_propertiesfile(propsFilePath, plugin);

        transport = ism_plugin_getControlChannelTransport();
        if (transport != NULL) {
            char xbuf [4096];
            concat_alloc_t buf = {xbuf, sizeof xbuf, 6};
            makePluginMap(&buf, plugin,1);
            transport->send(transport, buf.buf+6, buf.used-6, (PluginAction_UpdateProperties<<8)+0, SFLAG_FRAMESPACE);
            ism_plugin_freeControlChannelTransport();
            if(buf.inheap)
                ism_common_free(ism_memory_protocol_misc,buf.buf);
        }
    }
}



int ism_plugin_createPlugin(const char * pluginName, const char * zipFile, const char * propsFile, int overwrite) {
    char * argv[64] = {NULL};
    char * env[64] = {NULL};
    const char * cfg;
    int index = 1;
    int rc = ISMRC_OK;
    cfg = ism_common_getStringConfig("ConfigDir");
    if(cfg) {
        argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),"-c");
        argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),cfg);
    }
    argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),"-i");
    //Name
    argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),"-n");
    argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),pluginName);
    if(zipFile && zipFile[0]) {
    //Zip file
        argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),"-z");
        argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),zipFile);
    }
    if(propsFile && propsFile[0]) {
        //Props file
        argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),"-p");
        argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),propsFile);

    }
    if(overwrite) {
        //Overwrite
        argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),"-o");
    }
    rc = invokeScript("installPlugin", argv, env, 0);
    for(--index; index > 0; --index) {
        if(argv[index])
            ism_common_free(ism_memory_protocol_misc,argv[index]);
    }
    if((rc == ISMRC_OK) && (propsFile))
        updatePluginProperties(pluginName);
    return rc;
}

int ism_plugin_removePlugin(const char * pluginName) {
    if(pluginName && pluginName[0]) {
        char * argv[64] = {NULL};
        char * env[64] = {NULL};
        const char * cfg;
        int index = 1;
        int rc = ISMRC_OK;
        cfg = ism_common_getStringConfig("ConfigDir");
        if(cfg) {
            argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),"-c");
            argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),cfg);
        }
        argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),"-r");
        //Plugin name
        argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),"-n");
        argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),pluginName);
        rc = invokeScript("installPlugin", argv, env, 0);
        for(--index; index > 0; --index) {
            if(argv[index])
                ism_common_free(ism_memory_protocol_misc,argv[index]);
        }
        return rc;
    }
    ism_common_setError(ISMRC_NullArgument);
    return ISMRC_NullArgument;
}

int ism_protocol_isPluginServerRunning(void) {
    if(!pluginTerminated) {
        if(ism_plugin_getControlChannelTransport()){
            if(messagingStarted & 0x8) {
                ism_plugin_freeControlChannelTransport();
                return 1;
            }
            TRACE(7, "ism_protocol_isPluginServerRunning: Messaging is not started: messagingStarted=0x%x\n", messagingStarted);
            ism_plugin_freeControlChannelTransport();
            return 0;
        }
        TRACE(7, "ism_protocol_isPluginServerRunning: Control channel is not connected.\n");
        return 0;
    }
    TRACE(7, "ism_protocol_isPluginServerRunning: plugin is terminated.\n");
    return 0;
}

static int updatePlugins(void) {
    char * argv[64] = {NULL};
    char * env[64] = {NULL};
    const char * cfg;
    int index = 1;
    int rc = ISMRC_OK;
    cfg = ism_common_getStringConfig("ConfigDir");
    if(cfg) {
        argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),"-c");
        argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),cfg);
    }
    rc = invokeScript("updatePlugins", argv, env, 1);
    for(--index; index > 0; --index) {
        if(argv[index])
            ism_common_free(ism_memory_protocol_misc,argv[index]);
    }
    return rc;

}
